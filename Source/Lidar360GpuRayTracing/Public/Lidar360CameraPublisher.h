#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lidar360CameraPublisher.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UCLASS()
class LIDAR360GPURAYTRACING_API ALidar360CameraPublisher : public AActor
{
	GENERATED_BODY()

public:
	ALidar360CameraPublisher();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	FString TopicName = TEXT("rt/sensor_image");

	UPROPERTY(EditAnywhere, Category = "LiDAR360")
	FString FrameId = TEXT("camera_link");

	UPROPERTY(EditAnywhere, Category = "LiDAR360", meta = (ClampMin = "1", ClampMax = "60"))
	float PublishRateHz = 15.f;

	UPROPERTY(EditAnywhere, Category = "LiDAR360", meta = (ClampMin = "160", ClampMax = "1920"))
	int32 ImageWidth = 960;

	UPROPERTY(EditAnywhere, Category = "LiDAR360", meta = (ClampMin = "120", ClampMax = "1080"))
	int32 ImageHeight = 540;

protected:
	void OnTimer();
	void ShutdownDds();

	UPROPERTY(VisibleAnywhere, Category = "LiDAR360")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	FTimerHandle TimerHandle;
	int32 DdsWriter = 0;
	void* DdsImageSample = nullptr;
};
