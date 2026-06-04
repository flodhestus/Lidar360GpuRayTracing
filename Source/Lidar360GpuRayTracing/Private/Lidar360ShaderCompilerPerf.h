#pragma once

#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompilerCore.h"

inline void Lidar360ApplyPCD3DShaderCompilerPerfFlags(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
{
	if (Platform != SP_PCD3D_SM5 && Platform != SP_PCD3D_SM6)
	{
		return;
	}
	OutEnvironment.CompilerFlags.Add(CFLAG_ForceDXC);
	OutEnvironment.CompilerFlags.Add(CFLAG_Wave32);
	OutEnvironment.CompilerFlags.Add(CFLAG_CullBeforeFetch);
	OutEnvironment.CompilerFlags.Add(CFLAG_WarpCulling);
	if (Platform == SP_PCD3D_SM6)
	{
		OutEnvironment.CompilerFlags.Add(CFLAG_WaveOperations);
	}
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
	OutEnvironment.CompilerFlags.Add(CFLAG_ForceOptimization);
}
