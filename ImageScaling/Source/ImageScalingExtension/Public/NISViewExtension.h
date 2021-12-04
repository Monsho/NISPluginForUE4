#pragma once

#include "SceneViewExtension.h"
#include "RHIResources.h"
#include "NIS_Config.h"

class FNISViewExtension final : public FSceneViewExtensionBase
{
public:
	FNISViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {}

	// ISceneViewExtension interface
	void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override {}

	IMAGESCALINGEXTENSION_API void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	IMAGESCALINGEXTENSION_API void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;

	inline void SetCoefTextures(FRHITexture2D* Scaler, FRHITexture2D* Usm)
	{
		CoefScalerPtr = Scaler;
		CoefUsmPtr = Usm;
	}

private:
	FRHITexture2D*	CoefScalerPtr = nullptr;
	FRHITexture2D*	CoefUsmPtr = nullptr;
};
