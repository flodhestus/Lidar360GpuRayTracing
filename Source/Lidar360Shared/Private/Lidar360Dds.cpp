#include "Lidar360Dds.h"
#include "Async/Async.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

#if WITH_LIDAR360_DDS
THIRD_PARTY_INCLUDES_START
#include "dds/dds.h"
#include "sensor_msgs/msg/PointCloud2.h"
#include "sensor_msgs/msg/PointField.h"
THIRD_PARTY_INCLUDES_END
#endif

#if WITH_LIDAR360_DDS
namespace
{
	dds_entity_t GParticipant = 0;
	dds_entity_t GDomain = 0;

	dds_qos_t* MakeEndpointQos()
	{
		dds_qos_t* Qos = dds_create_qos();
		if (!Qos)
		{
			return nullptr;
		}
		dds_qset_reliability(Qos, DDS_RELIABILITY_BEST_EFFORT, DDS_INFINITY);
		const dds_data_representation_id_t Repr[] = { DDS_DATA_REPRESENTATION_XCDR2 };
		dds_qset_data_representation(Qos, 1, Repr);
		return Qos;
	}

	FString ConfigUri(const FString& PluginName)
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(FName(*PluginName));
		if (!Plugin.IsValid())
		{
			return FString();
		}
		const FString Path = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Config/CycloneDDS.xml"));
		return FString::Printf(TEXT("file:///%s"), *Path.Replace(TEXT("\\"), TEXT("/")));
	}
}
#endif

bool FLidar360Dds::Init(const FString& PluginName)
{
#if WITH_LIDAR360_DDS
	if (GParticipant > 0)
	{
		return true;
	}
	const FString Uri = ConfigUri(PluginName);
	if (!Uri.IsEmpty())
	{
		FPlatformMisc::SetEnvironmentVar(TEXT("CYCLONEDDS_URI"), *Uri);
	}
	GDomain = dds_create_domain(DDS_DOMAIN_DEFAULT, nullptr);
	GParticipant = dds_create_participant(DDS_DOMAIN_DEFAULT, nullptr, nullptr);
	return GParticipant > 0;
#else
	return false;
#endif
}

void FLidar360Dds::Shutdown()
{
#if WITH_LIDAR360_DDS
	if (GParticipant > 0)
	{
		dds_delete(GParticipant);
		GParticipant = 0;
	}
	if (GDomain > 0)
	{
		dds_delete(GDomain);
		GDomain = 0;
	}
#endif
}

bool FLidar360Dds::CreateWriter(const FString& TopicName, int32& OutWriter)
{
#if WITH_LIDAR360_DDS
	if (GParticipant <= 0)
	{
		return false;
	}
	dds_qos_t* Qos = MakeEndpointQos();
	FTCHARToUTF8 TopicUtf8(*TopicName);
	dds_entity_t Topic = dds_create_topic(
		GParticipant,
		&sensor_msgs_msg_PointCloud2_desc,
		TopicUtf8.Get(),
		Qos,
		nullptr);
	if (Topic < 0)
	{
		if (Qos) { dds_delete_qos(Qos); }
		return false;
	}
	dds_entity_t Writer = dds_create_writer(GParticipant, Topic, Qos, nullptr);
	if (Qos) { dds_delete_qos(Qos); }
	if (Writer < 0)
	{
		dds_delete(Topic);
		return false;
	}
	OutWriter = static_cast<int32>(Writer);
	return true;
#else
	return false;
#endif
}

bool FLidar360Dds::CreateReader(const FString& TopicName, int32& OutReader)
{
#if WITH_LIDAR360_DDS
	if (GParticipant <= 0)
	{
		return false;
	}
	dds_qos_t* Qos = MakeEndpointQos();
	FTCHARToUTF8 TopicUtf8(*TopicName);
	dds_entity_t Topic = dds_create_topic(
		GParticipant,
		&sensor_msgs_msg_PointCloud2_desc,
		TopicUtf8.Get(),
		Qos,
		nullptr);
	if (Topic < 0)
	{
		if (Qos) { dds_delete_qos(Qos); }
		return false;
	}
	dds_entity_t Reader = dds_create_reader(GParticipant, Topic, Qos, nullptr);
	if (Qos) { dds_delete_qos(Qos); }
	if (Reader < 0)
	{
		dds_delete(Topic);
		return false;
	}
	OutReader = static_cast<int32>(Reader);
	return true;
#else
	return false;
#endif
}

void FLidar360Dds::DestroyEndpoint(int32& Entity)
{
#if WITH_LIDAR360_DDS
	if (Entity > 0)
	{
		dds_delete(static_cast<dds_entity_t>(Entity));
		Entity = 0;
	}
#endif
}

sensor_msgs_msg_PointCloud2* FLidar360Dds::AllocSample()
{
#if WITH_LIDAR360_DDS
	auto* Sample = sensor_msgs_msg_PointCloud2__alloc();
	if (!Sample)
	{
		return nullptr;
	}
	Sample->fields._maximum = 4;
	Sample->fields._length = 4;
	Sample->fields._buffer = static_cast<sensor_msgs_msg_PointField*>(dds_alloc(4 * sizeof(sensor_msgs_msg_PointField)));
	Sample->fields._release = true;
	const uint32 MaxBytes = static_cast<uint32>(LIDAR360_MAX_POINTS) * LIDAR360_POINT_BYTES;
	Sample->data._maximum = MaxBytes;
	Sample->data._buffer = static_cast<uint8_t*>(dds_alloc(MaxBytes));
	Sample->data._release = true;
	return Sample;
#else
	return nullptr;
#endif
}

void FLidar360Dds::FreeSample(sensor_msgs_msg_PointCloud2* Sample)
{
#if WITH_LIDAR360_DDS
	if (Sample)
	{
		sensor_msgs_msg_PointCloud2_free(Sample, DDS_FREE_ALL);
	}
#endif
}

void FLidar360Dds::PublishAsync(int32 Writer, sensor_msgs_msg_PointCloud2* Sample, TFunction<void(bool)> OnDone)
{
#if WITH_LIDAR360_DDS
	if (Writer <= 0 || !Sample)
	{
		if (OnDone) { OnDone(false); }
		return;
	}
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [Writer, Sample, OnDone = MoveTemp(OnDone)]()
	{
		const bool bOk = dds_write(static_cast<dds_entity_t>(Writer), Sample) >= 0;
		if (OnDone)
		{
			AsyncTask(ENamedThreads::GameThread, [OnDone = MoveTemp(OnDone), bOk]() { OnDone(bOk); });
		}
	});
#else
	if (OnDone) { OnDone(false); }
#endif
}

bool FLidar360Dds::TakeLatest(int32 Reader, FLidar360SensorFrame& OutFrame)
{
#if WITH_LIDAR360_DDS
	if (Reader <= 0)
	{
		return false;
	}
	dds_sample_info_t Info;
	void* Buffer = sensor_msgs_msg_PointCloud2__alloc();
	const int32 N = dds_take(static_cast<dds_entity_t>(Reader), &Buffer, &Info, 1, 1);
	if (N <= 0 || !Buffer)
	{
		if (Buffer) { sensor_msgs_msg_PointCloud2_free(Buffer, DDS_FREE_ALL); }
		return false;
	}
	auto* Msg = static_cast<sensor_msgs_msg_PointCloud2*>(Buffer);
	const uint32 Len = Msg->data._length;
	OutFrame.Data.SetNumUninitialized(Len);
	if (Len > 0 && Msg->data._buffer)
	{
		FMemory::Memcpy(OutFrame.Data.GetData(), Msg->data._buffer, Len);
	}
	OutFrame.PointCount = FMath::Max(static_cast<int32>(Msg->width), static_cast<int32>(Len / LIDAR360_POINT_BYTES));
	sensor_msgs_msg_PointCloud2_free(Buffer, DDS_FREE_ALL);
	return OutFrame.PointCount > 0 && OutFrame.Data.Num() >= OutFrame.PointCount * LIDAR360_POINT_BYTES;
#else
	return false;
#endif
}
