#include "Lidar360GpuRayTracing.h"
#include "Lidar360RayTracingInterface.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "GlobalIlluminationPluginDelegates.h"

#if RHI_RAYTRACING
static FDelegateHandle GLidar360RtPassHandle;
static void Lidar360RtPass(bool& bEnabled) { bEnabled = true; }
#endif

void FLidar360GpuRayTracingModule::StartupModule()
{
	FString ShaderDir = FPaths::Combine(
		IPluginManager::Get().FindPlugin(TEXT("Lidar360GpuRayTracing"))->GetBaseDir(),
		TEXT("Shaders/Lidar360GpuRayTracing/Private"));
	AddShaderSourceDirectoryMapping(TEXT("/Lidar360GpuRayTracingShaders"), ShaderDir);
#if RHI_RAYTRACING
	GLidar360RtPassHandle = FGlobalIlluminationPluginDelegates::AnyRayTracingPassEnabled().AddStatic(&Lidar360RtPass);
#endif
}

void FLidar360GpuRayTracingModule::ShutdownModule()
{
#if RHI_RAYTRACING
	if (GLidar360RtPassHandle.IsValid())
	{
		FGlobalIlluminationPluginDelegates::AnyRayTracingPassEnabled().Remove(GLidar360RtPassHandle);
		GLidar360RtPassHandle.Reset();
	}
#endif
}

IMPLEMENT_MODULE(FLidar360GpuRayTracingModule, Lidar360GpuRayTracing)
