#pragma once

#include "CoreMinimal.h"

static constexpr int32 LIDAR360_GPU_STRIDE = 16;
static constexpr int32 LIDAR360_POINT_BYTES = 16;
static constexpr int32 LIDAR360_MAX_POINTS = 700000;
static constexpr float LIDAR360_MAX_RANGE_M = 200.f;
static constexpr float LIDAR360_MAX_RANGE_CM = LIDAR360_MAX_RANGE_M * 100.f;

struct FLidar360SensorFrame
{
	TArray<uint8> Data;
	int32 PointCount = 0;
};

static constexpr int32 LIDAR360_MAX_IMAGE_WIDTH = 1920;
static constexpr int32 LIDAR360_MAX_IMAGE_HEIGHT = 1080;
static constexpr int32 LIDAR360_IMAGE_BYTES = LIDAR360_MAX_IMAGE_WIDTH * LIDAR360_MAX_IMAGE_HEIGHT * 3;

struct FLidar360ImageFrame
{
	TArray<uint8> Data;
	int32 Width = 0;
	int32 Height = 0;
	int32 Step = 0;
};
