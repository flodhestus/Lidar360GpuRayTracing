using UnrealBuildTool;
using System.IO;

public class Lidar360GpuRayTracing : ModuleRules
{
	public Lidar360GpuRayTracing(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"RenderCore",
			"RHI",
			"Renderer",
			"Lidar360Shared"
		});

		PrivateDependencyModuleNames.AddRange(new[] { "Projects", "RenderCore", "Renderer", "RHI" });
	}
}
