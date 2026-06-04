#include "Lidar360Shared.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Lidar360PointCloud2Subscriber.h"

IMPLEMENT_MODULE(FLidar360SharedModule, Lidar360Shared)

static void SpawnDefaultSubscriber(UWorld* World)
{
	if (!World || World->WorldType != EWorldType::PIE)
	{
		return;
	}
	for (TActorIterator<ALidar360PointCloud2Subscriber> It(World); It; ++It)
	{
		return;
	}
	World->SpawnActor<ALidar360PointCloud2Subscriber>();
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
			}
		}
	});
}

void FLidar360SharedModule::ShutdownModule()
{
}
