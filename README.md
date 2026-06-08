# LiDAR 360 GPU Ray Tracing

Win64 UE 5.7 plugin: 360° LiDAR with **D3D12 hardware ray tracing**, CycloneDDS `sensor_msgs/PointCloud2`, and a live viewer on Play.

## What it does

On Play, spawns a publisher and subscriber on `rt/sensor_pointcloud` and opens **LiDAR360 GPU Point Cloud**.

## Requirements

| Item | Value |
|------|--------|
| Platform | Win64 |
| UE | 5.7 |
| GPU | RayTrace-capable (SM6) |
| Project settings | `r.RayTracing=True`, D3D12 |

## GPU pipeline

```
Timer
  → Ray-gen HLSL traces engine TLAS (view extension, before post-process)
  → Closest-hit writes hit distance
  → Ray-gen packs sensor-frame x/y/z/intensity on GPU
  → One GPU readback copy into DDS sample buffer
  → dds_write (async)
```

Each point is **16 bytes** (4 floats).

## Performance

Total rays per frame:

```
rays = NumRings × PointsPerRing
PointsPerRing = PointsPerSecond ÷ NumRings ÷ PublishRateHz
```

| Setting | Default | Effect |
|---------|---------|--------|
| `NumRings` | 128 | Vertical resolution |
| `PointsPerSecond` | 576000 | Horizontal density |
| `PublishRateHz` | 20 | Scan rate |
| `MaxRangeMeters` | 200 | Ray length |

**Rough cost per frame**

| Rays | Readback size | Notes |
|------|---------------|--------|
| ~225k | ~3.6 MB | Default at 20 Hz |
| ~576k | ~9.2 MB | Max default budget |
| 700k | ~11.2 MB | Codec buffer cap |

## Quick start

1. Copy into `YourProject/Plugins/`.
2. Enable **LiDAR 360 GPU Ray Tracing**.
3. Press play
