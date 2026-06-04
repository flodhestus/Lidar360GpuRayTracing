#include "Lidar360CameraPublisher.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Lidar360CameraCapture.h"
#include "Lidar360Dds.h"
#include "Lidar360ImageCodec.h"

#if WITH_LIDAR360_DDS
THIRD_PARTY_INCLUDES_START
#include "Image.h"
THIRD_PARTY_INCLUDES_END
#endif

ALidar360CameraPublisher::ALidar360CameraPublisher()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SetRootComponent(SceneCapture);
}

void ALidar360CameraPublisher::BeginPlay()
{
	Super::BeginPlay();
	if (!bEnabled)
	{
		return;
	}
	if (!FLidar360Dds::Init(TEXT("Lidar360GpuRayTracing")))
	{
		bEnabled = false;
		return;
	}
	if (!FLidar360Dds::CreateImageWriter(TopicName, DdsWriter))
	{
		bEnabled = false;
		return;
	}
	DdsImageSample = FLidar360Dds::AllocImageSample();
	if (DdsImageSample)
	{
		FLidar360ImageCodec::InitImageSample(
			*static_cast<sensor_msgs_msg_Image*>(DdsImageSample),
			ImageWidth,
			ImageHeight);
	}

	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitAutoFormat(ImageWidth, ImageHeight);
	RenderTarget->UpdateResourceImmediate(true);
	SceneCapture->TextureTarget = RenderTarget;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle, this, &ALidar360CameraPublisher::OnTimer,
		1.f / FMath::Max(1.f, PublishRateHz), true);
}

void ALidar360CameraPublisher::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
	ShutdownDds();
	Super::EndPlay(EndPlayReason);
}

void ALidar360CameraPublisher::ShutdownDds()
{
	FLidar360Dds::DestroyEndpoint(DdsWriter);
	if (DdsImageSample)
	{
		FLidar360Dds::FreeImageSample(static_cast<sensor_msgs_msg_Image*>(DdsImageSample));
		DdsImageSample = nullptr;
	}
}

void ALidar360CameraPublisher::OnTimer()
{
	if (!bEnabled || !RenderTarget || !DdsImageSample || DdsWriter <= 0)
	{
		return;
	}
	SceneCapture->CaptureScene();
	TArray<uint8> Rgb;
	int32 W = 0;
	int32 H = 0;
	if (!FLidar360CameraCapture::CaptureRgb8(RenderTarget, Rgb, W, H))
	{
		return;
	}
	auto* Sample = static_cast<sensor_msgs_msg_Image*>(DdsImageSample);
	if (!FLidar360ImageCodec::FillRgb8(*Sample, Rgb.GetData(), W, H, FrameId))
	{
		return;
	}
	FLidar360Dds::PublishImageAsync(DdsWriter, Sample);
}
