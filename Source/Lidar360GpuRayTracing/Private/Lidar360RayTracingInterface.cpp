#include "Lidar360RayTracingInterface.h"

#include "Async/Async.h"
#include "BuiltInRayTracingShaders.h"
#include "Lidar360ShaderCompilerPerf.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "DeferredShadingRenderer.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "RayTracing/RayTracingScene.h"
#include "RayTracingPayloadType.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIGPUReadback.h"
#include "RHI.h"
#include "SceneViewExtension.h"
#include "SceneRendering.h"
#include "ScenePrivate.h"
#include "ShaderParameterStruct.h"

DEFINE_LOG_CATEGORY_STATIC(LogLidar360, Log, All);

class FLidar360RG : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FLidar360RG);
	SHADER_USE_ROOT_PARAMETER_STRUCT(FLidar360RG, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		Lidar360ApplyPCD3DShaderCompilerPerfFlags(Parameters.Platform, OutEnvironment);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32)
	{
		return ERayTracingPayloadType::Default;
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(RaytracingAccelerationStructure, TLAS)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, OutPoints)
		SHADER_PARAMETER(FVector3f, LidarRayOrigin)
		SHADER_PARAMETER(FVector3f, LidarOriginWorldCm)
		SHADER_PARAMETER(FVector3f, PreViewTranslation)
		SHADER_PARAMETER(FVector3f, LidarForward)
		SHADER_PARAMETER(FVector3f, LidarRight)
		SHADER_PARAMETER(FVector3f, LidarUp)
		SHADER_PARAMETER(float, AzimuthMinDeg)
		SHADER_PARAMETER(float, AzimuthMaxDeg)
		SHADER_PARAMETER(float, ElevationMinDeg)
		SHADER_PARAMETER(float, ElevationMaxDeg)
		SHADER_PARAMETER(uint32, NumRings)
		SHADER_PARAMETER(uint32, PointsPerRing)
		SHADER_PARAMETER(float, MinRange)
		SHADER_PARAMETER(float, MaxRange)
		SHADER_PARAMETER(float, DefaultIntensity)
		SHADER_PARAMETER(float, FarNonHitDistance)
	END_SHADER_PARAMETER_STRUCT()
};

class FLidar360CHS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FLidar360CHS);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return ShouldCompileRayTracingShadersForProject(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		Lidar360ApplyPCD3DShaderCompilerPerfFlags(Parameters.Platform, OutEnvironment);
	}

	static ERayTracingPayloadType GetRayTracingPayloadType(const int32)
	{
		return ERayTracingPayloadType::Default;
	}

	FLidar360CHS() = default;
	FLidar360CHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
	}
};

IMPLEMENT_GLOBAL_SHADER(FLidar360RG, "/Lidar360GpuRayTracingShaders/Lidar360RayTracing.usf", "Lidar360RayTracingRG", SF_RayGen);
IMPLEMENT_SHADER_TYPE(, FLidar360CHS, TEXT("/Lidar360GpuRayTracingShaders/Lidar360RayTracingHit.usf"), TEXT("Lidar360RayTracingCHS"), SF_RayHitGroup);

struct FLidar360FrameRequest
{
	FLidar360RayTracingDispatchParams Params;
	TFunction<void(bool, int32)> Callback;
	uint8* DestBuffer = nullptr;
	int32 DestCapacityBytes = 0;
};

static bool Native360CanUsePipelineRayTracing()
{
	if (!GRHISupportsRayTracing || !GRHISupportsRayTracingShaders || !IsRayTracingEnabled())
	{
		return false;
	}
	if (GMaxRHIFeatureLevel < ERHIFeatureLevel::SM6)
	{
		return false;
	}
	static const IConsoleVariable* AllowPipeline = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.AllowPipeline"));
	return !AllowPipeline || AllowPipeline->GetInt() != 0;
}

static FRayTracingPipelineState* CreateNative360PipelineState(
	FRHICommandList& RHICmdList,
	const FGlobalShaderMap* ShaderMap,
	TShaderRef<FLidar360RG> RayGenShader,
	TShaderRef<FLidar360CHS> ClosestHitShader)
{
	FRayTracingPipelineStateInitializer Initializer;
	Initializer.MaxPayloadSizeInBytes = GetRayTracingPayloadTypeMaxSize(ERayTracingPayloadType::Default);
	Initializer.bAllowHitGroupIndexing = false;

	FRHIRayTracingShader* RayGenTable[] = { RayGenShader.GetRayTracingShader() };
	Initializer.SetRayGenShaderTable(RayGenTable);

	FRHIRayTracingShader* HitTable[] = { ClosestHitShader.GetRayTracingShader() };
	Initializer.SetHitGroupTable(HitTable);

	TShaderMapRef<FDefaultPayloadMS> MissShader(ShaderMap);
	FRHIRayTracingShader* MissTable[] = { MissShader.GetRayTracingShader() };
	Initializer.SetMissShaderTable(MissTable);

	return PipelineStateCache::GetAndOrCreateRayTracingPipelineState(RHICmdList, Initializer);
}

class FLidar360ViewExtension : public FWorldSceneViewExtension
{
public:
	FLidar360ViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld)
		: FWorldSceneViewExtension(AutoRegister, InWorld)
	{
	}

	virtual ~FLidar360ViewExtension() override
	{
		for (FRHIGPUBufferReadback* Readback : GpuReadbacks)
		{
			delete Readback;
		}
	}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override
	{
		FLidar360FrameRequest Request;
		{
			FScopeLock Lock(&PendingLock);
			if (!PendingRequest.IsSet())
			{
				return;
			}
			Request = MoveTemp(PendingRequest.GetValue());
			PendingRequest.Reset();
		}

		if (!InView.bIsViewInfo)
		{
			FinishRequest(false, 0, MoveTemp(Request.Callback));
			return;
		}

		FViewInfo& View = *const_cast<FViewInfo*>(static_cast<const FViewInfo*>(&InView));
		(void)Inputs;

		if (!View.Family || !View.Family->Scene || !Native360CanUsePipelineRayTracing())
		{
			FinishRequest(false, 0, MoveTemp(Request.Callback));
			return;
		}

		const FScene* Scene = View.Family->Scene->GetRenderScene();
		if (!Scene || !Scene->RayTracingScene.IsCreated())
		{
			FinishRequest(false, 0, MoveTemp(Request.Callback));
			return;
		}

		FRDGBufferSRVRef TLAS = Scene->RayTracingScene.GetLayerView(ERayTracingSceneLayer::Base);
		if (!TLAS)
		{
			FinishRequest(false, 0, MoveTemp(Request.Callback));
			return;
		}

		TShaderMapRef<FLidar360RG> RayGenShader(View.ShaderMap);
		TShaderMapRef<FLidar360CHS> ClosestHitShader(View.ShaderMap);
		TShaderMapRef<FDefaultPayloadMS> MissShader(View.ShaderMap);
		if (!RayGenShader.IsValid() || !ClosestHitShader.IsValid() || !MissShader.IsValid())
		{
			UE_LOG(LogLidar360, Error, TEXT("Native360: pipeline RT shaders missing (PCD3D_SM6, r.RayTracing=True, r.RayTracing.AllowPipeline=1)"));
			FinishRequest(false, 0, MoveTemp(Request.Callback));
			return;
		}

		const int32 TotalRays = FMath::Max(1, Request.Params.NumRings * Request.Params.PointsPerRing);
		const uint32 OutputFloatCount = static_cast<uint32>(TotalRays) * 4u;
		const uint32 OutputBytes = OutputFloatCount * sizeof(float);
		const uint32 DispatchWidth = FMath::DivideAndRoundUp(static_cast<uint32>(TotalRays), 128u) * 128u;

		FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
			FRDGBufferDesc::CreateStructuredDesc(sizeof(float), OutputFloatCount),
			TEXT("Lidar360Output"));

		FLidar360RG::FParameters* ShaderParams = GraphBuilder.AllocParameters<FLidar360RG::FParameters>();
		const FVector PreViewTranslation = View.ViewMatrices.GetPreViewTranslation();
		ShaderParams->TLAS = TLAS;
		ShaderParams->OutPoints = GraphBuilder.CreateUAV(OutputBuffer);
		ShaderParams->LidarRayOrigin = FVector3f(Request.Params.LidarOrigin + PreViewTranslation);
		ShaderParams->LidarOriginWorldCm = FVector3f(Request.Params.LidarOrigin);
		ShaderParams->PreViewTranslation = FVector3f(PreViewTranslation);
		ShaderParams->LidarForward = FVector3f(Request.Params.LidarForward.GetSafeNormal());
		ShaderParams->LidarRight = FVector3f(Request.Params.LidarRight.GetSafeNormal());
		ShaderParams->LidarUp = FVector3f(Request.Params.LidarUp.GetSafeNormal());
		ShaderParams->AzimuthMinDeg = Request.Params.AzimuthMinDeg;
		ShaderParams->AzimuthMaxDeg = Request.Params.AzimuthMaxDeg;
		ShaderParams->ElevationMinDeg = Request.Params.ElevationMinDeg;
		ShaderParams->ElevationMaxDeg = Request.Params.ElevationMaxDeg;
		ShaderParams->NumRings = static_cast<uint32>(FMath::Max(1, Request.Params.NumRings));
		ShaderParams->PointsPerRing = static_cast<uint32>(FMath::Max(1, Request.Params.PointsPerRing));
		ShaderParams->MinRange = Request.Params.MinRangeCm;
		ShaderParams->MaxRange = Request.Params.MaxRangeCm;
		ShaderParams->DefaultIntensity = Request.Params.DefaultIntensity;
		ShaderParams->FarNonHitDistance = Request.Params.FarNonHitDistanceCm;

		FRHIRayTracingScene* RHIRayTracingScene = Scene->RayTracingScene.GetRHIRayTracingScene();
		if (!RHIRayTracingScene)
		{
			FinishRequest(false, 0, MoveTemp(Request.Callback));
			return;
		}

		GraphBuilder.AddPass(
			RDG_EVENT_NAME("Lidar360PipelineTrace"),
			ShaderParams,
			ERDGPassFlags::Compute | ERDGPassFlags::NeverCull,
			[ShaderParams, RayGenShader, ClosestHitShader, ShaderMap = View.ShaderMap, RHIRayTracingScene, DispatchWidth](FRHIRayTracingCommandList& RHICmdList)
			{
				FRayTracingPipelineState* Pipeline = CreateNative360PipelineState(RHICmdList, ShaderMap, RayGenShader, ClosestHitShader);
				FRayTracingShaderBindingsWriter GlobalResources;
				SetShaderParameters(GlobalResources, RayGenShader, *ShaderParams);
				RHICmdList.SetRayTracingMissShader(RHIRayTracingScene, 0, Pipeline, 0, 0, nullptr, 0);
				RHICmdList.RayTraceDispatch(Pipeline, RayGenShader.GetRayTracingShader(), RHIRayTracingScene, GlobalResources, DispatchWidth, 1);
			});

		const int32 ReadbackIdx = NextReadbackIndex % 2;
		NextReadbackIndex = 1 - ReadbackIdx;
		if (!GpuReadbacks[ReadbackIdx])
		{
			GpuReadbacks[ReadbackIdx] = new FRHIGPUBufferReadback(TEXT("Lidar360Readback"));
		}
		FRHIGPUBufferReadback* Readback = GpuReadbacks[ReadbackIdx];
		AddEnqueueCopyPass(GraphBuilder, Readback, OutputBuffer, 0u);

		uint8* DestBuffer = Request.DestBuffer;
		const int32 DestCapacityBytes = Request.DestCapacityBytes;
		TFunction<void(bool, int32)> Callback = MoveTemp(Request.Callback);
		GraphBuilder.AddPass(
			RDG_EVENT_NAME("Lidar360Readback"),
			ERDGPassFlags::NeverCull,
			[Readback, OutputBytes, TotalRays, DestBuffer, DestCapacityBytes, Callback = MoveTemp(Callback)](FRHICommandListImmediate& RHICmdList) mutable
			{
				RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
				const double Deadline = FPlatformTime::Seconds() + 2.0;
				while (!Readback->IsReady() && FPlatformTime::Seconds() < Deadline)
				{
					FPlatformProcess::SleepNoStats(0.0005f);
				}
				if (!Readback->IsReady() || !DestBuffer || static_cast<int32>(OutputBytes) > DestCapacityBytes)
				{
					FinishRequest(false, 0, MoveTemp(Callback));
					return;
				}
				void* Data = Readback->Lock(OutputBytes);
				if (Data)
				{
					FMemory::Memcpy(DestBuffer, Data, OutputBytes);
					Readback->Unlock();
				}
				FinishRequest(true, TotalRays, MoveTemp(Callback));
			});
	}

	void SubmitRequest(
		const FLidar360RayTracingDispatchParams& Params,
		uint8* DestBuffer,
		int32 DestCapacityBytes,
		TFunction<void(bool, int32)> Callback)
	{
		FScopeLock Lock(&PendingLock);
		FLidar360FrameRequest Request;
		Request.Params = Params;
		Request.DestBuffer = DestBuffer;
		Request.DestCapacityBytes = DestCapacityBytes;
		Request.Callback = MoveTemp(Callback);
		PendingRequest = MoveTemp(Request);
	}

private:
	static void FinishRequest(bool bSuccess, int32 SlotCount, TFunction<void(bool, int32)> Callback)
	{
		AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), bSuccess, SlotCount]()
		{
			Callback(bSuccess, SlotCount);
		});
	}

	FCriticalSection PendingLock;
	TOptional<FLidar360FrameRequest> PendingRequest;
	FRHIGPUBufferReadback* GpuReadbacks[2] = { nullptr, nullptr };
	int32 NextReadbackIndex = 0;
};

static FCriticalSection GNative360ExtensionLock;
static TMap<TWeakObjectPtr<UWorld>, TSharedPtr<FLidar360ViewExtension>> GNative360Extensions;

void FLidar360RayTracingInterface::RegisterWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}
	FScopeLock Lock(&GNative360ExtensionLock);
	if (GNative360Extensions.Contains(World))
	{
		return;
	}
	TSharedRef<FLidar360ViewExtension> Extension = FSceneViewExtensions::NewExtension<FLidar360ViewExtension>(World);
	GNative360Extensions.Add(World, Extension);
}

void FLidar360RayTracingInterface::UnregisterWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}
	FScopeLock Lock(&GNative360ExtensionLock);
	GNative360Extensions.Remove(World);
}

void FLidar360RayTracingInterface::RequestFrame(
	UWorld* World,
	const FLidar360RayTracingDispatchParams& Params,
	uint8* DestBuffer,
	int32 DestCapacityBytes,
	TFunction<void(bool bSuccess, int32 SlotCount)> Callback)
{
	if (!World || !DestBuffer || DestCapacityBytes <= 0)
	{
		Callback(false, 0);
		return;
	}
	TSharedPtr<FLidar360ViewExtension> Extension;
	{
		FScopeLock Lock(&GNative360ExtensionLock);
		if (TSharedPtr<FLidar360ViewExtension>* Found = GNative360Extensions.Find(World))
		{
			Extension = *Found;
		}
	}
	if (!Extension.IsValid())
	{
		UE_LOG(LogLidar360, Warning, TEXT("Native360: no view extension for world"));
		Callback(false, 0);
		return;
	}
	Extension->SubmitRequest(Params, DestBuffer, DestCapacityBytes, MoveTemp(Callback));
}
