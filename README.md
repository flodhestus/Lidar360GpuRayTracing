# LiDAR 360 GPU Ray Tracing

Standalone Unreal Engine 5.7 plugin: **360° LiDAR** using **D3D12 hardware ray tracing** (Unreal `RHI_RAYTRACING` path on **Win64**), **`sensor_msgs/PointCloud2`** over **CycloneDDS**, and a **live point cloud viewer** on **Play**.

Repository: [github.com/flodhestus/Lidar360GpuRayTracing](https://github.com/flodhestus/Lidar360GpuRayTracing)

**No dependency** on Lidar360OptiX or Ros2SceneCamera. Each plugin is self-contained (own CycloneDDS + IDL). You may enable any combination in the same project; they communicate over DDS topics, not plugin links.

## On Play

| Feature | DDS topic (default) | Viewer |
|--------|------------------------|--------|
| GPU LiDAR publish + subscribe | `rt/sensor_pointcloud` | **LiDAR360 GPU Point Cloud** |

If **Lidar360OptiX** is also enabled in the project, this plugin skips GPU LiDAR pub/sub so only one LiDAR backend runs at a time.

## GPU path (Win64)

- **Ray generation + closest-hit** HLSL against the engine **TLAS**
- **FSceneViewExtension** before post-processing
- **Double-buffered GPU readback** of hit buffers
- Requires **SM6** and `r.RayTracing=True` (D3D12 RT)

This plugin does **not** implement a separate Vulkan or OptiX LiDAR path.

## Performance

Cost scales with **NumRings × PointsPerRing** and readback size (~16 B per point). Tune `PublishRateHz` and point budget for PIE.

## Quick start

1. Copy this folder into your project `Plugins/` directory.
2. Enable **LiDAR 360 GPU Ray Tracing** (Win64).
3. Press **Play** — publisher, subscriber, and viewer spawn automatically.

## Optional companions (separate repos, no plugin dependency)

- [Lidar360OptiX](https://github.com/flodhestus/Lidar360OptiX) — OptiX LiDAR  
- [Ros2SceneCamera](https://github.com/flodhestus/Ros2SceneCamera) — scene camera on `rt/sensor_image`  
