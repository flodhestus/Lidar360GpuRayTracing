# LiDAR 360 GPU Ray Tracing

Unreal Engine 5.7 plugin that traces a **360° LiDAR** with **hardware ray tracing** (DX12 / Vulkan), publishes **`sensor_msgs/PointCloud2`** over **CycloneDDS**, and opens a **live point cloud viewer** when you press **Play**.

Repository: [github.com/flodhestus/Lidar360GpuRayTracing](https://github.com/flodhestus/Lidar360GpuRayTracing)

This plugin also hosts the **`Ros2DdsShared` module** (CycloneDDS, codecs, PIE coordinator). **Lidar360OptiX** and **Ros2SceneCamera** depend on this plugin for shared DDS — there is no fourth plugin.

## On Play

| Feature | DDS topic | Viewer |
|--------|-----------|--------|
| LiDAR point cloud (pub + sub) | `rt/sensor_pointcloud` | **LiDAR360 GPU Point Cloud** |

Publisher and subscriber auto-spawn when this plugin is enabled and **OptiX LiDAR is not** (OptiX takes precedence if both are on).

## Running with other plugins (same DDS participant)

| Combination | LiDAR | Camera |
|-------------|-------|--------|
| This + **Ros2SceneCamera** | GPU RT | `rt/sensor_image` |
| **Lidar360OptiX** + **Ros2SceneCamera** | OptiX (enable OptiX, disable GPU LiDAR pub or let coordinator pick OptiX) | Image |
| All three enabled | **OptiX wins** for LiDAR | Image |

## GPU techniques

- **Ray generation + closest-hit** shaders against the engine **TLAS**
- **FSceneViewExtension** pass before post-processing
- Double-buffered **GPU readback** of hit buffers

## Quick start

1. Enable **LiDAR 360 GPU Ray Tracing** (Win64).
2. Optionally enable **Lidar360OptiX** and/or **Ros2SceneCamera** (they pull in shared DDS automatically).
3. Press **Play**.

## Related

- [Lidar360OptiX](https://github.com/flodhestus/Lidar360OptiX)  
- [Ros2SceneCamera](https://github.com/flodhestus/Ros2SceneCamera)  
