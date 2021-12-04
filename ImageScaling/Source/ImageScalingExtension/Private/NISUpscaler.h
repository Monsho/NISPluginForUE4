#pragma once

#include "NISViewData.h"
#include "PostProcess/PostProcessUpscale.h"

class FNISUpscaler final : public ISpatialUpscaler
{
public:
	FNISUpscaler(TArray<TSharedPtr<FNISViewData>> InViewData, FRHITexture2D* Scaler, FRHITexture2D* Usm);
	~FNISUpscaler();

	// ISpatialUpscaler interface
	const TCHAR* GetDebugName() const override { return TEXT("FNISUpscaler"); }

	ISpatialUpscaler* Fork_GameThread(const class FSceneViewFamily& ViewFamily) const override;
	FScreenPassTexture AddPasses(FRDGBuilder& GraphBuilder, const FViewInfo& View, const FInputs& PassInputs) const override;

public:
	//void CreateCoefResources();
	//void ReleaseCoefResources();

private:
	TSharedPtr<FNISViewData> GetDataForView(const FViewInfo& View) const;

	TArray<TSharedPtr<FNISViewData>> ViewData;

	FRHITexture2D*	CoefScalerPtr;
	FRHITexture2D*	CoefUsmPtr;
	//FTexture2DRHIRef	CoefScalerNIS;
	//FTexture2DRHIRef	CoefUsmNIS;
};