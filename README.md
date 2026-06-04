# LiDAR 360 GPU Ray Tracing

Unreal Engine 5.7 plugin that traces a **360° LiDAR** with **hardware ray tracing** (DX12 / Vulkan), publishes **`sensor_msgs/PointCloud2`** over **CycloneDDS**, and opens a **live point cloud viewer** when you press **Play**.

Repository: [github.com/flodhestus/Lidar360GpuRayTracing](https://github.com/flodhestus/Lidar360GpuRayTracing)

For a **normal scene camera** (not LiDAR), use the separate **[Ros2SceneCamera](https://github.com/flodhestus/Ros2SceneCamera)** plugin.

## On Play

| Feature | DDS topic | Viewer |
|--------|-----------|--------|
| LiDAR point cloud | `rt/sensor_pointcloud` | **LiDAR360 Point Cloud** |

## GPU techniques

- **Ray generation + closest-hit** shaders against the engine **TLAS**
- **FSceneViewExtension** pass before post-processing
- Optional **Gaussian range noise** in the raygen shader
- Double-buffered **GPU readback** of hit buffers

## Performance

Cost scales with **NumRings × PointsPerRing** per publish and readback size (~16 B per ray). Tune `PublishRateHz` and point budget for PIE. Requires **SM6** and `r.RayTracing=True`.

## Quick start

1. Enable **LiDAR 360 GPU Ray Tracing** (Win64).
2. Press **Play** — point cloud viewer opens when data is on the wire.
3. Optional: add **`ALidar360PointCloud2Publisher`** to publish GPU traces.

## Related

- [Lidar360OptiX](https://github.com/flodhestus/Lidar360OptiX)  
- [Ros2SceneCamera](https://github.com/flodhestus/Ros2SceneCamera)  
