#include "Lidar360PointCloud2Publisher.h"
#include "Lidar360Dds.h"
#include "Lidar360PointCloud2Codec.h"
#include "Lidar360RayTracingInterface.h"
#include "Ros2SensorCoordinator.h"

#if WITH_LIDAR360_DDS
THIRD_PARTY_INCLUDES_START
#include "sensor_msgs/msg/PointCloud2.h"
THIRD_PARTY_INCLUDES_END
#else
struct sensor_msgs_msg_PointCloud2;
#endif

ALidar360PointCloud2Publisher::ALidar360PointCloud2Publisher()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ALidar360PointCloud2Publisher::BeginPlay()
{
	Super::BeginPlay();
	if (!bEnabled || !FRos2SensorCoordinator::EnsureDdsInitialized())
	{
		bEnabled = false;
		return;
	}
	if (!FLidar360Dds::CreateWriter(TopicName, DdsWriter))
	{
		bEnabled = false;
		return;
	}
	DdsSample = FLidar360Dds::AllocSample();
	if (DdsSample)
	{
		FLidar360PointCloud2Codec::InitSampleFields(*static_cast<sensor_msgs_msg_PointCloud2*>(DdsSample));
	}
	FLidar360RayTracingInterface::RegisterWorld(GetWorld());
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle, this, &ALidar360PointCloud2Publisher::OnTimer,
		1.f / FMath::Max(0.5f, PublishRateHz), true);
}

void ALidar360PointCloud2Publisher::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
		FLidar360RayTracingInterface::UnregisterWorld(GetWorld());
	}
	ShutdownDds();
	Super::EndPlay(EndPlayReason);
}

void ALidar360PointCloud2Publisher::ShutdownDds()
{
	FLidar360Dds::DestroyEndpoint(DdsWriter);
	if (DdsSample)
	{
		FLidar360Dds::FreeSample(static_cast<sensor_msgs_msg_PointCloud2*>(DdsSample));
		DdsSample = nullptr;
	}
}

void ALidar360PointCloud2Publisher::OnTimer()
{
	if (bEnabled && FramesInFlight.load(std::memory_order_acquire) < MaxFramesInFlight)
	{
		ProcessFrame();
	}
}

void ALidar360PointCloud2Publisher::ProcessFrame()
{
	const int32 PointsPerRing = FMath::Max(1, PointsPerSecond / FMath::Max(1, NumRings) / FMath::Max(1, FMath::RoundToInt(PublishRateHz)));
	const int32 TotalRays = NumRings * PointsPerRing;

	FLidar360RayTracingDispatchParams Params;
	Params.LidarOrigin = GetActorLocation();
	Params.LidarForward = GetActorForwardVector();
	Params.LidarRight = GetActorRightVector();
	Params.LidarUp = GetActorUpVector();
	Params.NumRings = NumRings;
	Params.PointsPerRing = PointsPerRing;
	Params.MaxRangeCm = MaxRangeMeters * 100.f;
	Params.FarNonHitDistanceCm = Params.MaxRangeCm;
	Params.DefaultIntensity = DefaultIntensity;

	auto* Sample = static_cast<sensor_msgs_msg_PointCloud2*>(DdsSample);
	if (!Sample || !Sample->data._buffer || DdsWriter <= 0)
	{
		return;
	}

	FramesInFlight.fetch_add(1, std::memory_order_relaxed);
	TWeakObjectPtr<ALidar360PointCloud2Publisher> WeakThis(this);
	FLidar360RayTracingInterface::RequestFrame(
		GetWorld(),
		Params,
		Sample->data._buffer,
		static_cast<int32>(Sample->data._maximum),
		[WeakThis](bool bOk, int32 Slots)
		{
			ALidar360PointCloud2Publisher* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			if (bOk && Self->DdsSample && Self->DdsWriter > 0)
			{
				auto* Msg = static_cast<sensor_msgs_msg_PointCloud2*>(Self->DdsSample);
				const int32 N = FLidar360PointCloud2Codec::CommitSensorFrame(*Msg, Slots, Self->FrameId);
				if (N > 0)
				{
					FLidar360Dds::PublishAsync(Self->DdsWriter, Msg);
				}
			}
			Self->FramesInFlight.fetch_sub(1, std::memory_order_relaxed);
		});
}
