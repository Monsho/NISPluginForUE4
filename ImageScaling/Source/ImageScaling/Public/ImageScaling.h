// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RHIResources.h"
#include "Modules/ModuleManager.h"

class FNISViewExtension;

class FImageScalingModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FNISViewExtension, ESPMode::ThreadSafe> NISViewExtension;
};
