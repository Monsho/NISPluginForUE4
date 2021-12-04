#include "NISUpscaler.h"
#include "NIS_Config.h"

static float GNIS_Sharpness = 0.5f;
static FAutoConsoleVariableRef CVarNISSharpness(
	TEXT("r.NVIDIA.NIS.Sharpness"),
	GNIS_Sharpness,
	TEXT("Sharpness parameter for NIS."),
	ECVF_RenderThreadSafe);

DECLARE_GPU_STAT(NVIDIAImageScalingPass)

BEGIN_SHADER_PARAMETER_STRUCT(FNISPassParameters, )
	SHADER_PARAMETER(float, kDetectRatio)
	SHADER_PARAMETER(float, kDetectThres)
	SHADER_PARAMETER(float, kMinContrastRatio)
	SHADER_PARAMETER(float, kRatioNorm)

	SHADER_PARAMETER(float, kContrastBoost)
	SHADER_PARAMETER(float, kEps)
	SHADER_PARAMETER(float, kSharpStartY)
	SHADER_PARAMETER(float, kSharpScaleY)

	SHADER_PARAMETER(float, kSharpStrengthMin)
	SHADER_PARAMETER(float, kSharpStrengthScale)
	SHADER_PARAMETER(float, kSharpLimitMin)
	SHADER_PARAMETER(float, kSharpLimitScale)

	SHADER_PARAMETER(float, kScaleX)
	SHADER_PARAMETER(float, kScaleY)

	SHADER_PARAMETER(float, kDstNormX)
	SHADER_PARAMETER(float, kDstNormY)
	SHADER_PARAMETER(float, kSrcNormX)
	SHADER_PARAMETER(float, kSrcNormY)

	SHADER_PARAMETER(uint32, kInputViewportOriginX)
	SHADER_PARAMETER(uint32, kInputViewportOriginY)
	SHADER_PARAMETER(uint32, kInputViewportWidth)
	SHADER_PARAMETER(uint32, kInputViewportHeight)

	SHADER_PARAMETER(uint32, kOutputViewportOriginX)
	SHADER_PARAMETER(uint32, kOutputViewportOriginY)
	SHADER_PARAMETER(uint32, kOutputViewportWidth)
	SHADER_PARAMETER(uint32, kOutputViewportHeight)

	SHADER_PARAMETER(float, reserved0)
	SHADER_PARAMETER(float, reserved1)

	SHADER_PARAMETER_SAMPLER(SamplerState, samplerLinearClamp)
	SHADER_PARAMETER_RDG_TEXTURE(Texture2D, in_texture)
	SHADER_PARAMETER_TEXTURE(Texture2D, coef_scaler)
	SHADER_PARAMETER_TEXTURE(Texture2D, coef_usm)
END_SHADER_PARAMETER_STRUCT()

///
/// NIS COMPUTE SHADER
///
class FNISCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FNISCS);
	SHADER_USE_PARAMETER_STRUCT(FNISCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_INCLUDE(FNISPassParameters, NIS)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, out_texture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		NISOptimizer Optimizer = NISOptimizer();
		OutEnvironment.SetDefine(TEXT("NIS_THREAD_GROUP_SIZE"), Optimizer.GetOptimalThreadGroupSize());
	}
};

IMPLEMENT_GLOBAL_SHADER(FNISCS, "/Plugin/ImageScaling/Private/PostProcessNIS.usf", "MainCS", SF_Compute);

///
/// NIS Copy PIXEL SHADER
///
class FNISCopyPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FNISCopyPS);
	SHADER_USE_PARAMETER_STRUCT(FNISCopyPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
	}
};

IMPLEMENT_GLOBAL_SHADER(FNISCopyPS, "/Plugin/ImageScaling/Private/PostProcessNIS_Copy.usf", "MainPS", SF_Pixel);


FNISUpscaler::FNISUpscaler(TArray<TSharedPtr<FNISViewData>> InViewData, FRHITexture2D* Scaler, FRHITexture2D* Usm)
	: ViewData(InViewData)
	, CoefScalerPtr(Scaler)
	, CoefUsmPtr(Usm)
{
}

FNISUpscaler::~FNISUpscaler()
{
}


ISpatialUpscaler* FNISUpscaler::Fork_GameThread(const class FSceneViewFamily& ViewFamily) const
{
	// the object we return here will get deleted by UE4 when the scene view tears down, so we need to instantiate a new one every frame.
	return new FNISUpscaler(ViewData, CoefScalerPtr, CoefUsmPtr);
}

FScreenPassTexture FNISUpscaler::AddPasses(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FInputs& PassInputs) const
{
	RDG_GPU_STAT_SCOPE(GraphBuilder, NVIDIAImageScalingPass);
	check(PassInputs.SceneColor.IsValid());

	TSharedPtr<FNISViewData> Data = GetDataForView(View);

	// create resources.
	auto OutputDesc = PassInputs.SceneColor.Texture->Desc;
	OutputDesc.Reset();
	OutputDesc.Extent = View.UnscaledViewRect.Max;
	OutputDesc.ClearValue = FClearValueBinding::Black;
	OutputDesc.Flags = TexCreate_ShaderResource | TexCreate_UAV | TexCreate_RenderTargetable;
	Data->UpscaleTexture = FScreenPassTexture(PassInputs.SceneColor);
	Data->UpscaleTexture.Texture = GraphBuilder.CreateTexture(OutputDesc, TEXT("NIS_Upscale_Output"), ERDGTextureFlags::MultiFrame);
	Data->UpscaleTexture.ViewRect = View.UnscaledViewRect;

	auto OutputViewport = FScreenPassTextureViewport(Data->UpscaleTexture);
	auto InputViewport = FScreenPassTextureViewport(PassInputs.SceneColor);

	// render upscale.
	NISConfig config;
	NVScalerUpdateConfig(config, GNIS_Sharpness,
		InputViewport.Rect.Min.X, InputViewport.Rect.Min.Y,
		InputViewport.Rect.Width(), InputViewport.Rect.Height(),
		PassInputs.SceneColor.Texture->Desc.Extent.X, PassInputs.SceneColor.Texture->Desc.Extent.Y,
		0, 0,
		OutputViewport.Rect.Width(), OutputViewport.Rect.Height(),
		Data->UpscaleTexture.Texture->Desc.Extent.X, Data->UpscaleTexture.Texture->Desc.Extent.Y,
		NISHDRMode::None);

	FScreenPassRenderTarget Output = PassInputs.OverrideOutput;

	{
		FNISCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FNISCS::FParameters>();

		PassParameters->NIS.kDetectRatio = config.kDetectRatio;
		PassParameters->NIS.kDetectThres = config.kDetectThres;
		PassParameters->NIS.kMinContrastRatio = config.kMinContrastRatio;
		PassParameters->NIS.kRatioNorm = config.kRatioNorm;
		PassParameters->NIS.kContrastBoost = config.kContrastBoost;
		PassParameters->NIS.kEps = config.kEps;
		PassParameters->NIS.kSharpStartY = config.kSharpStartY;
		PassParameters->NIS.kSharpScaleY = config.kSharpScaleY;
		PassParameters->NIS.kSharpStrengthMin = config.kSharpStrengthMin;
		PassParameters->NIS.kSharpStrengthScale = config.kSharpStrengthScale;
		PassParameters->NIS.kSharpLimitMin = config.kSharpLimitMin;
		PassParameters->NIS.kSharpLimitScale = config.kSharpLimitScale;
		PassParameters->NIS.kScaleX = config.kScaleX;
		PassParameters->NIS.kScaleY = config.kScaleY;
		PassParameters->NIS.kDstNormX = config.kDstNormX;
		PassParameters->NIS.kDstNormY = config.kDstNormY;
		PassParameters->NIS.kSrcNormX = config.kSrcNormX;
		PassParameters->NIS.kSrcNormY = config.kSrcNormY;
		PassParameters->NIS.kInputViewportOriginX = config.kInputViewportOriginX;
		PassParameters->NIS.kInputViewportOriginY = config.kInputViewportOriginY;
		PassParameters->NIS.kInputViewportWidth = config.kInputViewportWidth;
		PassParameters->NIS.kInputViewportHeight = config.kInputViewportHeight;
		PassParameters->NIS.kOutputViewportOriginX = config.kOutputViewportOriginX;
		PassParameters->NIS.kOutputViewportOriginY = config.kOutputViewportOriginY;
		PassParameters->NIS.kOutputViewportWidth = config.kOutputViewportWidth;
		PassParameters->NIS.kOutputViewportHeight = config.kOutputViewportHeight;
		PassParameters->NIS.reserved0 = config.reserved0;
		PassParameters->NIS.reserved1 = config.reserved1;

		PassParameters->NIS.in_texture = PassInputs.SceneColor.Texture;
		PassParameters->NIS.samplerLinearClamp = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->NIS.coef_scaler = CoefScalerPtr;
		PassParameters->NIS.coef_usm = CoefUsmPtr;
		PassParameters->out_texture = GraphBuilder.CreateUAV(Data->UpscaleTexture.Texture);

		NISOptimizer Optimizer = NISOptimizer();

		TShaderMapRef<FNISCS> ComputeShaderNIS(View.ShaderMap);
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("NVIDIA Image Scaling %dx%d -> %dx%d (CS)"
				, InputViewport.Rect.Width(), InputViewport.Rect.Height()
				, OutputViewport.Rect.Width(), OutputViewport.Rect.Height()),
			ComputeShaderNIS,
			PassParameters,
			FComputeShaderUtils::GetGroupCount(OutputViewport.Rect.Size(), FIntPoint(Optimizer.GetOptimalBlockWidth(), Optimizer.GetOptimalBlockHeight())));
	}

	// render copy.
	// NOTE: UE4's final target do NOT have UAV flag. NIS can NOT render directly to the final target as it is CS only.
	{
		FNISCopyPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FNISCopyPS::FParameters>();
		PassParameters->InputTexture = Data->UpscaleTexture.Texture;
		PassParameters->RenderTargets[0] = FRenderTargetBinding(Output.Texture, ERenderTargetLoadAction::ENoAction);

		TShaderMapRef<FNISCopyPS> PixelShader(View.ShaderMap);

		AddDrawScreenPass(GraphBuilder,
			RDG_EVENT_NAME("NVIDIA Image Scaling Copy %dx%d (PS)"
				, OutputViewport.Rect.Width(), OutputViewport.Rect.Height()),
			View, OutputViewport, OutputViewport,
			PixelShader, PassParameters,
			EScreenPassDrawFlags::None
		);
	}

	FScreenPassTexture FinalOutput = Output;
	return MoveTemp(FinalOutput);
}

TSharedPtr<FNISViewData> FNISUpscaler::GetDataForView(const FViewInfo& View) const
{
	for (int i = 0; i < View.Family->Views.Num(); i++)
	{
		if (View.Family->Views[i] == &View)
		{
			return ViewData[i];
		}
	}
	return nullptr;
}
