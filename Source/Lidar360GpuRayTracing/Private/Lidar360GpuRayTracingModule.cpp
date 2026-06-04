#include "Lidar360GpuRayTracing.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Lidar360PointCloud2Publisher.h"
#include "Lidar360RayTracingInterface.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Ros2SensorCoordinator.h"
#include "GlobalIlluminationPluginDelegates.h"

#if RHI_RAYTRACING
static FDelegateHandle GLidar360RtPassHandle;
static void Lidar360RtPass(bool& bEnabled) { bEnabled = true; }
#endif

static void SpawnGpuLidarPublisher(UWorld* World)
{
	if (!World || World->WorldType != EWorldType::PIE || !FRos2SensorCoordinator::ShouldRunGpuLidar())
	{
		return;
	}
	for (TActorIterator<ALidar360PointCloud2Publisher> It(World); It; ++It)
	{
		return;
	}
	World->SpawnActor<ALidar360PointCloud2Publisher>();
}

void FLidar360GpuRayTracingModule::StartupModule()
{
	FString ShaderDir = FPaths::Combine(
		IPluginManager::Get().FindPlugin(TEXT("Lidar360GpuRayTracing"))->GetBaseDir(),
		TEXT("Shaders/Lidar360GpuRayTracing/Private"));
	AddShaderSourceDirectoryMapping(TEXT("/Lidar360GpuRayTracingShaders"), ShaderDir);
#if RHI_RAYTRACING
	GLidar360RtPassHandle = FGlobalIlluminationPluginDelegates::AnyRayTracingPassEnabled().AddStatic(&Lidar360RtPass);
#endif

	FWorldDelegates::OnPIEStarted.AddLambda([](const bool)
	{
		if (!GEngine)
		{
			return;
		}
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.World() && Ctx.WorldType == EWorldType::PIE)
			{
				FRos2SensorCoordinator::EnsureDdsInitialized();
				SpawnGpuLidarPublisher(Ctx.World());
			}
		}
	});
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
