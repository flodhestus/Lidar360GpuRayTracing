#include "Lidar360CameraSubscriber.h"
#include "Lidar360Dds.h"
#include "Lidar360Win32ImageViewport.h"
#include "Async/Async.h"

ALidar360CameraSubscriber::ALidar360CameraSubscriber()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ALidar360CameraSubscriber::BeginPlay()
{
	Super::BeginPlay();
	if (!bEnabled)
	{
		return;
	}
	if (!FLidar360Dds::Init(PluginName))
	{
		bEnabled = false;
		return;
	}
	if (!FLidar360Dds::CreateImageReader(TopicName, DdsReader))
	{
		bEnabled = false;
		return;
	}
	Viewport = MakeShared<FLidar360Win32ImageViewport>();
	Viewport->StartViewport(ViewportTitle);
	GetWorld()->GetTimerManager().SetTimer(PollTimer, this, &ALidar360CameraSubscriber::PollDds, 0.033f, true);
}

void ALidar360CameraSubscriber::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PollTimer);
	}
	if (Viewport.IsValid())
	{
		Viewport->StopViewport();
		Viewport.Reset();
	}
	FLidar360Dds::DestroyEndpoint(DdsReader);
	Super::EndPlay(EndPlayReason);
}

void ALidar360CameraSubscriber::PollDds()
{
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis = TWeakObjectPtr<ALidar360CameraSubscriber>(this)]()
	{
		ALidar360CameraSubscriber* Self = WeakThis.Get();
		if (!Self || !Self->bEnabled || Self->DdsReader <= 0)
		{
			return;
		}
		FLidar360ImageFrame Frame;
		if (!FLidar360Dds::TakeLatestImage(Self->DdsReader, Frame))
		{
			return;
		}
		AsyncTask(ENamedThreads::GameThread, [WeakThis, Frame]() mutable
		{
			if (ALidar360CameraSubscriber* Sub = WeakThis.Get())
			{
				Sub->OnFrame(Frame);
			}
		});
	});
}

void ALidar360CameraSubscriber::OnFrame(const FLidar360ImageFrame& Frame)
{
	if (Viewport.IsValid() && Frame.Data.Num() > 0)
	{
		Viewport->SubmitImage(Frame.Data.GetData(), Frame.Width, Frame.Height);
	}
}
