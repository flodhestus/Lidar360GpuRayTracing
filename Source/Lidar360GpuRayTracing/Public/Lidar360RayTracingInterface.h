#pragma once

#include "CoreMinimal.h"
#include "Lidar360Types.h"

struct FLidar360RayTracingDispatchParams
{
	FVector LidarOrigin = FVector::ZeroVector;
	FVector LidarForward = FVector::ForwardVector;
	FVector LidarRight = FVector::RightVector;
	FVector LidarUp = FVector::UpVector;
	float AzimuthMinDeg = -180.f;
	float AzimuthMaxDeg = 180.f;
	float ElevationMinDeg = -25.f;
	float ElevationMaxDeg = 15.f;
	int32 NumRings = 128;
	int32 PointsPerRing = 1;
	float MinRangeCm = 0.f;
	float MaxRangeCm = LIDAR360_MAX_RANGE_CM;
	float DefaultIntensity = 1.f;
	float FarNonHitDistanceCm = LIDAR360_MAX_RANGE_CM;
	bool bGaussianNoiseEnabled = true;
	float NoiseMeanM = 0.f;
	float NoiseStDevBaseM = 0.02f;
	float NoiseStDevRisePerMeter = 0.002f;
};

class LIDAR360GPURAYTRACING_API FLidar360RayTracingInterface
{
public:
	static void RegisterWorld(UWorld* World);
	static void UnregisterWorld(UWorld* World);
	static void RequestFrame(
		UWorld* World,
		const FLidar360RayTracingDispatchParams& Params,
		uint8* DestBuffer,
		int32 DestCapacityBytes,
		TFunction<void(bool bSuccess, int32 SlotCount)> Callback);
};
