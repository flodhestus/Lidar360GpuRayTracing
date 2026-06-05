#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lidar360Types.h"
#include <atomic>
#include "Lidar360PointCloud2Publisher.generated.h"

UCLASS()
class LIDAR360GPURAYTRACING_API ALidar360PointCloud2Publisher : public AActor
{
	GENERATED_BODY()

public:
	ALidar360PointCloud2Publisher();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	FString TopicName = TEXT("rt/sensor_pointcloud");

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	FString FrameId = TEXT("lidar_link");

	UPROPERTY(EditAnywhere, Category = "LiDAR360", meta = (ClampMin = "0.5", ClampMax = "100"))
	float PublishRateHz = 20.f;

	UPROPERTY(EditAnywhere, Category = "LiDAR360", meta = (ClampMin = "1", ClampMax = "256"))
	int32 NumRings = 128;

	UPROPERTY(EditAnywhere, Category = "LiDAR360", meta = (ClampMin = "10000", ClampMax = "1200000"))
	int32 PointsPerSecond = 576000;

	UPROPERTY(EditAnywhere, Category = "LiDAR360", meta = (ClampMin = "0", ClampMax = "200"))
	float MaxRangeMeters = LIDAR360_MAX_RANGE_M;

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	float DefaultIntensity = 1.f;

protected:
	void OnTimer();
	void ProcessFrame();
	void ShutdownDds();

	FTimerHandle TimerHandle;
	int32 DdsWriter = 0;
	void* DdsSample = nullptr;
	std::atomic<int32> FramesInFlight{0};
	static constexpr int32 MaxFramesInFlight = 2;
};
