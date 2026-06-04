#include "Lidar360Shared.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Lidar360PointCloud2Subscriber.h"
#include "Lidar360CameraSubscriber.h"

IMPLEMENT_MODULE(FLidar360SharedModule, Lidar360Shared)

static bool IsOptiXPluginEnabled()
{
	const TSharedPtr<IPlugin> OptiX = IPluginManager::Get().FindPlugin(TEXT("Lidar360OptiX"));
	return OptiX.IsValid() && OptiX->IsEnabled();
}

static void SpawnDefaultSubscriber(UWorld* World)
{
	if (!World || World->WorldType != EWorldType::PIE || IsOptiXPluginEnabled())
	{
		return;
	}
	for (TActorIterator<ALidar360PointCloud2Subscriber> It(World); It; ++It)
	{
		return;
	}
	World->SpawnActor<ALidar360PointCloud2Subscriber>();
}

static void SpawnCameraSubscriber(UWorld* World)
{
	if (!World || World->WorldType != EWorldType::PIE)
	{
		return;
	}
	for (TActorIterator<ALidar360CameraSubscriber> It(World); It; ++It)
	{
		return;
	}
	World->SpawnActor<ALidar360CameraSubscriber>();
}

void FLidar360SharedModule::StartupModule()
{
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
				SpawnDefaultSubscriber(Ctx.World());
				SpawnCameraSubscriber(Ctx.World());
			}
		}
	});
}

void FLidar360SharedModule::ShutdownModule()
{
}
