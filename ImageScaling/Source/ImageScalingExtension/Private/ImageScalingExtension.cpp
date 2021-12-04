// Copyright Epic Games, Inc. All Rights Reserved.

#include "ImageScalingExtension.h"
#include "Interfaces/IPluginManager.h"
#include "NISViewExtension.h"
#include "NISUpscaler.h"

#define LOCTEXT_NAMESPACE "FImageScalingExtensionModule"

void FImageScalingExtensionModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ImageScaling"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/ImageScaling"), PluginShaderDir);
}

void FImageScalingExtensionModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FImageScalingExtensionModule, ImageScalingExtension)