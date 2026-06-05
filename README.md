# LiDAR 360 GPU Ray Tracing

Win64 UE 5.7 plugin: 360° LiDAR with **D3D12 hardware ray tracing**, CycloneDDS `sensor_msgs/PointCloud2`, and a live viewer on Play.

Repo: [github.com/flodhestus/Lidar360GpuRayTracing](https://github.com/flodhestus/Lidar360GpuRayTracing)

## What it does

On Play, spawns a publisher and subscriber on `rt/sensor_pointcloud` and opens **LiDAR360 GPU Point Cloud**.

If **Lidar360OptiX** is also enabled, this plugin stays off LiDAR so only one backend runs.

## Requirements

| Item | Value |
|------|--------|
| Platform | Win64 |
| UE | 5.7 |
| GPU | RT-capable (SM6) |
| Project settings | `r.RayTracing=True`, D3D12 |

No CUDA or OptiX required.

## GPU pipeline

```
Timer
  → Ray-gen HLSL traces engine TLAS (view extension, before post-process)
  → Closest-hit writes hit distance
  → Ray-gen packs sensor-frame x/y/z/intensity on GPU
  → One GPU readback copy into DDS sample buffer
  → dds_write (async)
```

Each point is **16 bytes** (4 floats). Rays use the same scan pattern as OptiX: rings × points per ring over azimuth/elevation.

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

Tips for smooth PIE:

- Lower `PublishRateHz` first (10–15 Hz is often enough for debug).
- Reduce `PointsPerSecond` before `NumRings` if you need less horizontal detail.
- Keep `MaxFramesInFlight` at 2 (built-in) — do not raise publish rate above what readback can finish.

## Quick start

1. Copy into `YourProject/Plugins/`.
2. Enable **LiDAR 360 GPU Ray Tracing**.
3. Press Play.

## Related plugins

- [Lidar360OptiX](https://github.com/flodhestus/Lidar360OptiX) — NVIDIA OptiX LiDAR  
- [Ros2SceneCamera](https://github.com/flodhestus/Ros2SceneCamera) — RGB camera on `rt/sensor_image`  
