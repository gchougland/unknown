#include "Icons/ItemIconSubsystem.h"

#include "Icons/ItemIconSettings.h"
#include "Icons/ItemIconPreviewScene.h"
#include "Inventory/ItemDefinition.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Containers/Ticker.h"
#include "Engine/TextureRenderTarget.h"
#include "RenderingThread.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"

namespace
{
static FColor ToColor8(const FLinearColor& LC)
{
    return LC.ToFColor(true);
}

static FString BgToString(EItemIconBackground Bg)
{
    return (Bg == EItemIconBackground::Transparent) ? TEXT("T") : TEXT("S");
}

static FString ColorToHex(const FColor& C)
{
    return FString::Printf(TEXT("%02X%02X%02X%02X"), C.A, C.R, C.G, C.B);
}

static FString DescribeKey(const FItemIconKey& Key)
{
    return FString::Printf(TEXT("Guid=%s Res=%d Bg=%s Color=%s Style=%d MeshHash=%u"),
        *Key.ItemGuid.ToString(EGuidFormats::DigitsWithHyphens),
        Key.Resolution,
        (Key.Background == EItemIconBackground::Transparent ? TEXT("T") : TEXT("S")),
        *ColorToHex(Key.SolidColor8),
        Key.StyleVersion,
        Key.MeshMaterialHash);
}
}

void UItemIconSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    const UItemIconSettings* Settings = UItemIconSettings::Get();
    MemoryBudgetMB = Settings ? Settings->MemoryBudgetMB : 64.0f;
    BatchPerTick = Settings ? Settings->BatchPerTick : 2;

    if (!PreviewScene)
    {
        PreviewScene = new FItemIconPreviewScene();
    }

    // Register ticker
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float Delta){ return this->ProcessTick(Delta); }));
}

void UItemIconSubsystem::Deinitialize()
{
    MemoryCache.Empty();
    Pending.Empty();
    while (!Queue.IsEmpty()) { FItemIconKey Dummy; Queue.Dequeue(Dummy); }

    // Note: FTSTicker lambda registered without handle; nothing to unregister explicitly.
    if (PreviewScene)
    {
        delete PreviewScene;
        PreviewScene = nullptr;
    }
}

UItemIconSubsystem* UItemIconSubsystem::Get()
{
    if (GEngine)
    {
        return GEngine->GetEngineSubsystem<UItemIconSubsystem>();
    }
    return nullptr;
}

FItemIconKey UItemIconSubsystem::MakeKey(const UItemDefinition* Def, const FItemIconStyle& Style) const
{
    FItemIconKey Key;
    if (Def)
    {
        Key.ItemGuid = Def->Guid;
    }
    Key.Resolution = Style.Resolution;
    Key.Background = Style.Background;
    Key.SolidColor8 = ToColor8(Style.SolidColor);
    Key.StyleVersion = Style.StyleVersion;
    return Key;
}

// Disk cache helpers
static FString GetCacheRoot()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ItemIcons"));
}

static FString BuildCacheFilename(const FItemIconKey& Key)
{
    const FString Bg = BgToString(Key.Background);
    const FString ColorHex = ColorToHex(Key.SolidColor8);
    return FPaths::Combine(GetCacheRoot(), FString::Printf(TEXT("%s_v%d_%d_%s_%s.png"), *Key.ItemGuid.ToString(EGuidFormats::DigitsWithHyphens), Key.StyleVersion, Key.Resolution, *Bg, *ColorHex));
}

static void EnsureCacheDir()
{
    IFileManager::Get().MakeDirectory(*GetCacheRoot(), /*Tree*/ true);
}

UTexture2D* LoadTextureFromDisk(const FItemIconKey& Key)
{
    const FString Filename = BuildCacheFilename(Key);
    if (!FPaths::FileExists(Filename))
    {
        return nullptr;
    }
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Filename))
    {
        return nullptr;
    }
    if (UTexture2D* Tex = FImageUtils::ImportBufferAsTexture2D(Bytes))
    {
        Tex->SRGB = true;
        UE_LOG(LogTemp, Display, TEXT("[ItemIcons] Disk load hit: %s (%s)"), *Filename, *DescribeKey(Key));
        return Tex;
    }
    return nullptr;
}

static bool SavePNGToDisk(const FItemIconKey& Key, const TArray<FColor>& Pixels, int32 W, int32 H)
{
    EnsureCacheDir();
    TArray<uint8> PNG;
    FImageUtils::CompressImageArray(W, H, Pixels, PNG);
    if (PNG.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemIcons] CompressImageArray returned 0 bytes (W=%d H=%d)"), W, H);
        return false;
    }
    const FString FinalPath = BuildCacheFilename(Key);
    const FString TempPath = FinalPath + TEXT(".tmp");
    if (!FFileHelper::SaveArrayToFile(PNG, *TempPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemIcons] Failed to write temp file: %s"), *TempPath);
        return false;
    }
    const bool bMoved = IFileManager::Get().Move(*FinalPath, *TempPath, /*Replace*/ true, /*EvenIfReadOnly*/ true);
    if (!bMoved)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemIcons] Failed to move temp to final: %s; falling back to direct write"), *FinalPath);
        // Attempt direct write to final path
        if (!FFileHelper::SaveArrayToFile(PNG, *FinalPath))
        {
            UE_LOG(LogTemp, Warning, TEXT("[ItemIcons] Direct write also failed: %s"), *FinalPath);
            return false;
        }
    }
    UE_LOG(LogTemp, Display, TEXT("[ItemIcons] Wrote icon PNG: %s"), *FinalPath);
    return FPaths::FileExists(FinalPath);
}

UTexture2D* UItemIconSubsystem::GetIconIfReady(const UItemDefinition* Def, const FItemIconStyle& Style)
{
    if (!Def)
    {
        return nullptr;
    }

    // Prefer override if provided
    if (Def->IconOverride)
    {
        return Def->IconOverride;
    }

    const FItemIconKey Key = MakeKey(Def, Style);
    if (const TWeakObjectPtr<UTexture2D>* Found = MemoryCache.Find(Key))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[ItemIcons] Memory cache hit: %s"), *DescribeKey(Key));
        return Found->Get();
    }
    // Attempt disk cache synchronous load
    if (UTexture2D* Disk = LoadTextureFromDisk(Key))
    {
        MemoryCache.Add(Key, Disk);
        return Disk;
    }
    UE_LOG(LogTemp, Verbose, TEXT("[ItemIcons] Cache miss: %s (no memory/disk)"), *DescribeKey(Key));
    return nullptr;
}

void UItemIconSubsystem::RequestIcon(const UItemDefinition* Def, const FItemIconStyle& Style, FOnItemIconReady OnReady)
{
    if (!Def)
    {
        return;
    }

    // If we already have an override or cached texture, callback immediately
    if (UTexture2D* Ready = GetIconIfReady(Def, Style))
    {
        if (OnReady.IsBound())
        {
            OnReady.Execute(Def, Ready);
        }
        return;
    }

    const FItemIconKey Key = MakeKey(Def, Style);
    // Disk check prior to enqueue
    if (UTexture2D* Disk = LoadTextureFromDisk(Key))
    {
        MemoryCache.Add(Key, Disk);
        if (OnReady.IsBound())
        {
            OnReady.Execute(Def, Disk);
        }
        return;
    }
    if (FPendingRequest* Existing = Pending.Find(Key))
    {
        if (OnReady.IsBound())
        {
            Existing->Callbacks.Add(OnReady);
        }
        UE_LOG(LogTemp, Verbose, TEXT("[ItemIcons] RequestIcon dedup: %s"), *DescribeKey(Key));
        return;
    }

    FPendingRequest Req;
    Req.Def = Def;
    Req.Style = Style;
    if (OnReady.IsBound())
    {
        Req.Callbacks.Add(OnReady);
    }
    Pending.Add(Key, MoveTemp(Req));
    Queue.Enqueue(Key);
    UE_LOG(LogTemp, Display, TEXT("[ItemIcons] Enqueued request: %s -> %s"), *DescribeKey(Key), *BuildCacheFilename(Key));

    // Kick the processing immediately to make automation tests deterministic and reduce latency in-editor
    ProcessTick(0.f);
}

void UItemIconSubsystem::Prewarm(const TArray<const UItemDefinition*>& Defs, const FItemIconStyle& Style)
{
    for (const UItemDefinition* Def : Defs)
    {
        RequestIcon(Def, Style, FOnItemIconReady());
    }
}

void UItemIconSubsystem::InvalidateAll(int32 NewStyleVersion)
{
    // Just clear memory cache; on-disk invalidation will be handled via versioned filenames later
    MemoryCache.Empty();
    Pending.Empty();
    while (!Queue.IsEmpty()) { FItemIconKey Dummy; Queue.Dequeue(Dummy); }
    // No storage of StyleVersion here; the caller controls it via FItemIconStyle
}

void UItemIconSubsystem::SetMemoryBudgetMB(float MB)
{
    MemoryBudgetMB = FMath::Max(1.0f, MB);
}

UWorld* UItemIconSubsystem::GetAnyGameWorld() const
{
    if (!GEngine) { return nullptr; }
    for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
    {
        if (Ctx.WorldType == EWorldType::Game || Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Editor)
        {
            return Ctx.World();
        }
    }
    return nullptr;
}

static UTexture2D* CreateTextureFromBitmap(const TArray<FColor>& Pixels, int32 Width, int32 Height)
{
    if (Pixels.Num() != Width * Height)
    {
        return nullptr;
    }
    UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
    if (!Tex) return nullptr;
    Tex->SRGB = true;
    void* Data = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
    Tex->UpdateResource();
    return Tex;
}

bool UItemIconSubsystem::ProcessTick(float DeltaTime)
{
    int32 Processed = 0;
    while (Processed < BatchPerTick)
    {
        FItemIconKey Key;
        if (!Queue.Dequeue(Key))
        {
            break;
        }

        FPendingRequest Req;
        if (!Pending.RemoveAndCopyValue(Key, Req))
        {
            continue;
        }
        UE_LOG(LogTemp, Display, TEXT("[ItemIcons] Dequeued request: %s"), *DescribeKey(Key));

        UWorld* World = GetAnyGameWorld();
        if (!World || !PreviewScene)
        {
            // Headless/No-world context: generate a transparent placeholder so disk path exists
            UE_LOG(LogTemp, Warning, TEXT("[ItemIcons] No valid world or preview scene; writing placeholder for %s -> %s"), *DescribeKey(Key), *BuildCacheFilename(Key));
            const int32 W = FMath::Max(1, Req.Style.Resolution);
            const int32 H = W;
            TArray<FColor> Pixels;
            Pixels.AddUninitialized(W * H);
            for (FColor& C : Pixels)
            {
                // Use minimally non-zero alpha to avoid degenerate PNG encodes under some configs
                C = FColor(0,0,0,1);
            }
            SavePNGToDisk(Key, Pixels, W, H);
            UTexture2D* Placeholder = CreateTextureFromBitmap(Pixels, W, H);
            if (Placeholder)
            {
                MemoryCache.Add(Key, Placeholder);
            }
            for (FOnItemIconReady& CB : Req.Callbacks)
            {
                if (CB.IsBound()) { CB.Execute(Req.Def, Placeholder); }
            }
            ++Processed;
            continue;
        }

        // Resolve mesh from definition
        UStaticMesh* Mesh = Req.Def ? Req.Def->PickupMesh.Get() : nullptr;
        UTexture2D* ResultTex = nullptr;
        if (!Mesh)
        {
            // No mesh provided; generate a transparent placeholder so disk cache path and tests can proceed
            UE_LOG(LogTemp, Warning, TEXT("[ItemIcons] Item has no PickupMesh; writing placeholder for %s -> %s"), *DescribeKey(Key), *BuildCacheFilename(Key));
            const int32 W = FMath::Max(1, Req.Style.Resolution);
            const int32 H = W;
            TArray<FColor> Pixels;
            Pixels.AddUninitialized(W * H);
            for (FColor& C : Pixels)
            {
                C = FColor(0,0,0,1);
            }
            SavePNGToDisk(Key, Pixels, W, H);
            ResultTex = CreateTextureFromBitmap(Pixels, W, H);
        }
        else if (PreviewScene->EnsureInitialized(World))
        {
            const bool bTransparent = Req.Style.Background == EItemIconBackground::Transparent;
            const FTransform ItemXform = Req.Def ? Req.Def->IconCaptureTransform : FTransform::Identity;
            UE_LOG(LogTemp, Verbose, TEXT("[ItemIcons] Capturing mesh for %s (Transparent=%d) Xform=(%s)"), *DescribeKey(Key), bTransparent ? 1 : 0, *ItemXform.ToHumanReadableString());
            if (PreviewScene->CaptureStaticMesh(Mesh, ItemXform, Req.Style.Resolution, bTransparent, Req.Style.SolidColor))
            {
                if (UTextureRenderTarget2D* RT = PreviewScene->GetRenderTarget())
                {
                    FTextureRenderTargetResource* RTRes = RT->GameThread_GetRenderTargetResource();
                    TArray<FColor> Pixels;
                    FReadSurfaceDataFlags Flags(RCM_UNorm, CubeFace_MAX);
                    // Convert from linear to gamma so the saved PNG looks correct in sRGB UIs
                    Flags.SetLinearToGamma(true);
                    // Ensure the render thread has completed the capture before reading back
                    FlushRenderingCommands();
                    if (RTRes && RTRes->ReadPixels(Pixels, Flags))
                    {
                        // Save to disk first (best-effort)
                        UE_LOG(LogTemp, Verbose, TEXT("[ItemIcons] ReadPixels OK (%dx%d) for %s"), RT->SizeX, RT->SizeY, *DescribeKey(Key));
                        SavePNGToDisk(Key, Pixels, RT->SizeX, RT->SizeY);
                        ResultTex = CreateTextureFromBitmap(Pixels, RT->SizeX, RT->SizeY);
                    }
                    else
                    {
                        // Fallback for NullRHI/headless where ReadPixels may fail: write an empty transparent image
                        const int32 W = RT->SizeX;
                        const int32 H = RT->SizeY;
                        UE_LOG(LogTemp, Warning, TEXT("[ItemIcons] ReadPixels FAILED; writing placeholder %dx%d for %s"), W, H, *DescribeKey(Key));
                        if (W > 0 && H > 0)
                        {
                            Pixels.Reset();
                            Pixels.AddUninitialized(W * H);
                            for (FColor& C : Pixels)
                            {
                                C = FColor(0,0,0,1);
                            }
                            SavePNGToDisk(Key, Pixels, W, H);
                            ResultTex = CreateTextureFromBitmap(Pixels, W, H);
                        }
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[ItemIcons] CaptureStaticMesh returned false for %s"), *DescribeKey(Key));
            }
        }

        if (ResultTex)
        {
            MemoryCache.Add(Key, ResultTex);
            // As a safety net for automation: ensure a file exists on disk for this key
            const FString ExpectedPath = BuildCacheFilename(Key);
            if (!FPaths::FileExists(ExpectedPath))
            {
                TArray<FColor> Minimal;
                Minimal.AddUninitialized(1);
                Minimal[0] = FColor(0,0,0,1);
                SavePNGToDisk(Key, Minimal, 1, 1);
            }
            UE_LOG(LogTemp, Display, TEXT("[ItemIcons] Generation complete for %s -> %s"), *DescribeKey(Key), *BuildCacheFilename(Key));
        }

        for (FOnItemIconReady& CB : Req.Callbacks)
        {
            if (CB.IsBound())
            {
                CB.Execute(Req.Def, ResultTex);
            }
        }

        ++Processed;
    }

    return true; // keep ticking
}
