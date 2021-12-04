#pragma once

#include "ShaderParameterMacros.h"
#include "ScreenPass.h"

struct FNISViewData
{
	bool bInitialized = false;

	FScreenPassTexture UpscaleTexture;
};
