#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lidar360Types.h"
#include "Lidar360CameraSubscriber.generated.h"

UCLASS()
class LIDAR360SHARED_API ALidar360CameraSubscriber : public AActor
{
	GENERATED_BODY()

public:
	ALidar360CameraSubscriber();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	FString TopicName = TEXT("rt/sensor_image");

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	FString PluginName = TEXT("Lidar360GpuRayTracing");

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	FString ViewportTitle = TEXT("LiDAR360 Camera");

protected:
	void PollDds();
	void OnFrame(const FLidar360ImageFrame& Frame);

	FTimerHandle PollTimer;
	TSharedPtr<class FLidar360Win32ImageViewport> Viewport;
	int32 DdsReader = 0;
};
