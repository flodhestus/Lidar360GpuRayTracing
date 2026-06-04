# LiDAR 360 GPU Ray Tracing

Unreal Engine 5.7 plugin that traces a **360° LiDAR** with **hardware ray tracing** (DX12 / Vulkan), publishes **`sensor_msgs/PointCloud2`** over **CycloneDDS**, and opens a **live point cloud viewer** when you press **Play**.

Repository: [github.com/flodhestus/Lidar360GpuRayTracing](https://github.com/flodhestus/Lidar360GpuRayTracing)

Depends on **[Ros2DdsShared](https://github.com/flodhestus/Ros2DdsShared)** for DDS and the point-cloud viewer. Runs alongside **[Ros2SceneCamera](https://github.com/flodhestus/Ros2SceneCamera)** on the same DDS participant.

## On Play

| Feature | DDS topic | Viewer |
|--------|-----------|--------|
| LiDAR point cloud (pub + sub) | `rt/sensor_pointcloud` | **LiDAR360 GPU Point Cloud** |

Publisher and subscriber auto-spawn when this plugin is enabled and **OptiX LiDAR is not** (OptiX takes precedence if both are on).

## GPU techniques

- **Ray generation + closest-hit** shaders against the engine **TLAS**
- **FSceneViewExtension** pass before post-processing
- Optional **Gaussian range noise** in the raygen shader
- Double-buffered **GPU readback** of hit buffers

## Performance

Cost scales with **NumRings × PointsPerRing** per publish and readback size (~16 B per ray). Tune `PublishRateHz` and point budget for PIE. Requires **SM6** and `r.RayTracing=True`.

Pair with the camera plugin for synchronized scene color + LiDAR without a second DDS stack.

## Quick start

1. Enable **ROS2 DDS Shared** and **LiDAR 360 GPU Ray Tracing** (Win64).
2. Optionally enable **ROS2 Scene Camera** for `rt/sensor_image`.
3. Press **Play** — LiDAR publisher, subscriber, and viewers start automatically.

## Related

- [Ros2DdsShared](https://github.com/flodhestus/Ros2DdsShared)  
- [Lidar360OptiX](https://github.com/flodhestus/Lidar360OptiX)  
- [Ros2SceneCamera](https://github.com/flodhestus/Ros2SceneCamera)  
