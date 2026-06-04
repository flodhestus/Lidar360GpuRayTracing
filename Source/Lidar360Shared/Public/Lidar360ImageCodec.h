#pragma once

#include "CoreMinimal.h"

#if WITH_LIDAR360_DDS
struct sensor_msgs_msg_Image;
#endif

class LIDAR360SHARED_API FLidar360ImageCodec
{
public:
#if WITH_LIDAR360_DDS
	static void InitImageSample(sensor_msgs_msg_Image& Sample, int32 MaxWidth, int32 MaxHeight);
	static bool FillRgb8(
		sensor_msgs_msg_Image& Sample,
		const uint8* Rgb,
		int32 Width,
		int32 Height,
		const FString& FrameId);
#endif
};
