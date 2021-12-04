#include "NISViewExtension.h"
#include "NISUpscaler.h"

static TAutoConsoleVariable<int32> CVarEnableNIS(
	TEXT("r.NVIDIA.NIS.Enabled"),
	0,
	TEXT("Enable NVIDIA Image Scaling."),
	ECVF_RenderThreadSafe);

void FNISViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	if (InViewFamily.GetFeatureLevel() >= ERHIFeatureLevel::SM5 && CVarEnableNIS.GetValueOnAnyThread() > 0)
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			static auto CVarEnableEditorScreenPercentageOverride = IConsoleManager::Get().FindConsoleVariable(TEXT("Editor.OverrideDPIBasedEditorViewportScaling"));
			CVarEnableEditorScreenPercentageOverride->Set(1);
		}
#endif

		TArray<TSharedPtr<FNISViewData>> ViewData;

		for (int i = 0; i < InViewFamily.Views.Num(); i++)
		{
			const FSceneView* InView = InViewFamily.Views[i];
			if (ensure(InView))
			{
				FNISViewData* Data = new FNISViewData();
				ViewData.Add(TSharedPtr<FNISViewData>(Data));
			}
		}

		InViewFamily.SetPrimarySpatialUpscalerInterface(new FNISUpscaler(ViewData, CoefScalerPtr, CoefUsmPtr));
	}
}

void FNISViewExtension::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	if (CVarEnableNIS.GetValueOnAnyThread() > 0)
	{
	}
}
