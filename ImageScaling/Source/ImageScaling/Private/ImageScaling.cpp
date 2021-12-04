// Copyright Epic Games, Inc. All Rights Reserved.

#include "ImageScaling.h"
#include "Interfaces/IPluginManager.h"
#include "NISViewExtension.h"
#include "NIS_Config.h"

#define LOCTEXT_NAMESPACE "FImageScalingModule"

namespace
{
	FTexture2DRHIRef	CoefScalerNIS;
	FTexture2DRHIRef	CoefUsmNIS;

	void CreateCoefResources()
	{
		class FNISResourceBulkData : public FResourceBulkDataInterface
		{
		public:
			FNISResourceBulkData(const float Value[kPhaseCount][kFilterSize])
			{
				Data.AddUninitialized(kFilterSize * kPhaseCount);
				for (int32 y = 0; y < kPhaseCount; y++)
				{
					for (int32 x = 0; x < kFilterSize; x += 4)
					{
						Data[y * kFilterSize + x + 0] = Value[y][x + 0];
						Data[y * kFilterSize + x + 1] = Value[y][x + 1];
						Data[y * kFilterSize + x + 2] = Value[y][x + 2];
						Data[y * kFilterSize + x + 3] = Value[y][x + 3];
					}
				}
			}

			/** 
			* @return ptr to the resource memory which has been preallocated
			*/
			virtual const void* GetResourceBulkData() const override
			{
				return Data.GetData();
			}

			/** 
			* @return size of resource memory
			*/
			virtual uint32 GetResourceBulkDataSize() const override
			{
				return Data.Num() * sizeof(float);
			}

			/**
			* Free memory after it has been used to initialize RHI resource 
			*/
			virtual void Discard() override
			{
				Data.Empty();
			}

		private:
			TArray<float>	Data;
		};

		if (!CoefScalerNIS.IsValid())
		{
			FNISResourceBulkData BulkData(coef_scale);
			FRHIResourceCreateInfo CreateInfo;
			CreateInfo.BulkData = &BulkData;
			CoefScalerNIS = RHICreateTexture2D(
				kFilterSize / 4,
				kPhaseCount,
				EPixelFormat::PF_A32B32G32R32F,
				1,
				1,
				TexCreate_ShaderResource,
				CreateInfo);
		}
		if (!CoefUsmNIS.IsValid())
		{
			FNISResourceBulkData BulkData(coef_usm);
			FRHIResourceCreateInfo CreateInfo;
			CreateInfo.BulkData = &BulkData;
			CoefUsmNIS = RHICreateTexture2D(
				kFilterSize / 4,
				kPhaseCount,
				EPixelFormat::PF_A32B32G32R32F,
				1,
				1,
				TexCreate_ShaderResource,
				CreateInfo);
		}
	}

	void ReleaseCoefResources()
	{
		CoefScalerNIS.SafeRelease();
		CoefUsmNIS.SafeRelease();
	}
}

void FImageScalingModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	CreateCoefResources();
	NISViewExtension = FSceneViewExtensions::NewExtension<FNISViewExtension>();
	NISViewExtension->SetCoefTextures(CoefScalerNIS, CoefUsmNIS);
}

void FImageScalingModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	NISViewExtension = nullptr;
	ReleaseCoefResources();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FImageScalingModule, ImageScaling)