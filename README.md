# LiDAR 360 GPU Ray Tracing

Unreal Engine 5.7 plugin that traces a **360° LiDAR** with **hardware ray tracing** (DX12 / Vulkan), publishes **`sensor_msgs/PointCloud2`** and **`sensor_msgs/Image`** over **CycloneDDS**, and opens **live Win32 viewer windows** when you press **Play** in the editor.

Repository: [github.com/flodhestus/Lidar360GpuRayTracing](https://github.com/flodhestus/Lidar360GpuRayTracing)

## What you get on Play

| Feature | DDS topic (default) | Viewer window |
|--------|---------------------|---------------|
| LiDAR point cloud | `rt/sensor_pointcloud` | **LiDAR360 Point Cloud** |
| Scene camera (RGB) | `rt/sensor_image` | **LiDAR360 Camera** |

Actors are spawned automatically in **PIE** (editor Play). You can also place publishers/subscribers manually in a level.

## GPU techniques

### 1. Native ray tracing LiDAR (`Lidar360RayTracing.usf`)

- **Ray generation shader** casts one ray per ring × azimuth sample from a configurable origin and orientation.
- Uses the engine **TLAS** (top-level acceleration structure) so hits are against the full rendered scene.
- **Closest-hit shader** returns hit distance; misses use a far-range placeholder.
- Optional **Gaussian range noise** in the shader (distance-dependent standard deviation).
- Output: structured buffer of `(x, y, z, intensity)` in world space (cm), then packed to ROS **PointCloud2** (`x, y, z, intensity` float fields).

**Pipeline integration**

- `FSceneViewExtension` injects a pass in **`PrePostProcessPass_RenderThread`** so tracing sees the same scene as the main view.
- Registers with **`FGlobalIlluminationPluginDelegates::AnyRayTracingPassEnabled`** so the RT pipeline stays active while the plugin is in use.
- Double-buffered **GPU readback** avoids stalling the render thread every frame.

**Requirements**

- `r.RayTracing=True`, `r.RayTracing.AllowPipeline=1`, **SM6**, Win64.

### 2. Compute shader camera (`Lidar360CameraCapture.usf`)

- **`USceneCaptureComponent2D`** renders **Final Color LDR** into a render target (default **960×540**).
- **Compute shader** (`8×8` thread groups) samples the RT and writes packed **RGB8** pixels.
- Published as ROS **`sensor_msgs/Image`** with encoding **`rgb8`** on `rt/sensor_image`.
- If the compute shader is unavailable, a CPU **ReadPixels** fallback is used.

## Performance notes

| Workload | Typical cost drivers | Practical guidance |
|----------|----------------------|--------------------|
| **LiDAR trace** | `NumRings × PointsPerRing` rays per publish; TLAS traversal; GPU→CPU readback | Default publisher targets ~20 Hz. Lower `PointsPerSecond` or rings for editor tests. Full 128×4500-class scans are heavy—tune `PublishRateHz` and point budget. |
| **Readback** | One structured buffer copy per frame (~16 B × ray count) | Ping-pong readback hides some latency; still scales linearly with ray count. |
| **DDS publish** | Best-effort write on a background thread | No custom QoS in code—rely on `Config/CycloneDDS.xml`. |
| **Camera** | Scene capture + compute + RGB buffer publish | Default **15 Hz**, 960×540. Reduce resolution or rate for lighter loads. |
| **Viewers** | Separate Win32/OpenGL threads | Cheap vs GPU trace; polling DDS at ~60 Hz (cloud) / ~30 Hz (image). |

**Order-of-magnitude** (hardware-dependent): hundreds of thousands of rays per frame at 20 Hz is realistic on a mid/high-end RT GPU; millions of points per second requires careful tuning and may need lower publish rates in PIE.

## DDS / ROS2

- **CycloneDDS** with types generated from IDL (`PointCloud2`, `Image`, `Header`, …).
- Config: `Config/CycloneDDS.xml` (topic discovery, socket buffers, tracing).
- Writers/readers use **default QoS** (no extra QoS in application code).

## Modules

| Module | Role |
|--------|------|
| **Lidar360Shared** | DDS, codecs, point cloud + **image** subscribers, Win32 viewers |
| **Lidar360GpuRayTracing** | RT LiDAR, compute camera, publishers |

## Quick start

1. Copy the plugin into your project `Plugins/` folder.
2. Enable **LiDAR 360 GPU Ray Tracing** (Win64).
3. Ensure ray tracing is enabled in project settings / console.
4. Press **Play** — point cloud and camera windows should appear when data is published.
5. Optional: place **`ALidar360PointCloud2Publisher`** / **`ALidar360CameraPublisher`** for custom rates and transforms.

## Main actors

- `ALidar360PointCloud2Publisher` — GPU LiDAR → PointCloud2  
- `ALidar360PointCloud2Subscriber` — PointCloud2 → 3D point viewer  
- `ALidar360CameraPublisher` — Scene capture + compute → Image  
- `ALidar360CameraSubscriber` — Image → 2D viewer  

## Related plugin

**[Lidar360OptiX](https://github.com/flodhestus/Lidar360OptiX)** — NVIDIA OptiX path for LiDAR (depends on this plugin for shared DDS/viewers).
