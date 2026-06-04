#include "Ros2SensorCoordinator.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Lidar360Dds.h"
#include "Lidar360PointCloud2Subscriber.h"

static bool IsPluginEnabled(const FName PluginName)
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
	return Plugin.IsValid() && Plugin->IsEnabled();
}

bool FRos2SensorCoordinator::IsOptiXLidarEnabled()
{
	return IsPluginEnabled(TEXT("Lidar360OptiX"));
}

bool FRos2SensorCoordinator::IsGpuLidarEnabled()
{
	return IsPluginEnabled(TEXT("Lidar360GpuRayTracing"));
}

bool FRos2SensorCoordinator::IsSceneCameraEnabled()
{
	return IsPluginEnabled(TEXT("Ros2SceneCamera"));
}

bool FRos2SensorCoordinator::ShouldRunOptiXLidar()
{
	return IsOptiXLidarEnabled();
}

bool FRos2SensorCoordinator::ShouldRunGpuLidar()
{
	return IsGpuLidarEnabled() && !IsOptiXLidarEnabled();
}

bool FRos2SensorCoordinator::EnsureDdsInitialized()
{
	return FLidar360Dds::Init();
}

static void SpawnGpuLidarSubscriber(UWorld* World)
{
	if (!World || World->WorldType != EWorldType::PIE || !FRos2SensorCoordinator::ShouldRunGpuLidar())
	{
		return;
	}
	for (TActorIterator<ALidar360PointCloud2Subscriber> It(World); It; ++It)
	{
		return;
	}
	ALidar360PointCloud2Subscriber* Sub = World->SpawnActor<ALidar360PointCloud2Subscriber>();
	if (Sub)
	{
		Sub->PluginName = TEXT("Lidar360GpuRayTracing");
		Sub->ViewportTitle = TEXT("LiDAR360 GPU Point Cloud");
	}
}

void FRos2SensorCoordinator::OnPieWorldStarted(UWorld* World)
{
	if (!World || World->WorldType != EWorldType::PIE)
	{
		return;
	}
	EnsureDdsInitialized();
	SpawnGpuLidarSubscriber(World);
}
