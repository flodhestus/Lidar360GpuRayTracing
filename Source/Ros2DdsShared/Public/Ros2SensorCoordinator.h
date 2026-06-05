#pragma once

#include "CoreMinimal.h"

class UWorld;

class ROS2DDSSHARED_API FRos2SensorCoordinator
{
public:
	static bool IsOptiXLidarEnabled();
	static bool IsGpuLidarEnabled();
	static bool IsSceneCameraEnabled();
	static bool ShouldRunOptiXLidar();
	static bool ShouldRunGpuLidar();
	static bool EnsureDdsInitialized();
	static void OnPieWorldStarted(UWorld* World);
};
