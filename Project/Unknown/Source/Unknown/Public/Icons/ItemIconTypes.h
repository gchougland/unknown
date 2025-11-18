#pragma once

#include "CoreMinimal.h"

// Background options for icon rendering
enum class EItemIconBackground : uint8
{
    Transparent,
    SolidColor
};

// Style parameters that affect the visual outcome and cache key
struct FItemIconStyle
{
    int32 Resolution = 256; // Width = Height
    EItemIconBackground Background = EItemIconBackground::Transparent;
    // Used when Background == SolidColor
    FLinearColor SolidColor = FLinearColor(0.125f, 0.125f, 0.125f, 1.0f);
    // Increment to invalidate all cached images on disk
    int32 StyleVersion = 1;
};

// Cache key: combines item identity with style parameters
struct FItemIconKey
{
    FGuid ItemGuid;
    int32 Resolution = 256;
    EItemIconBackground Background = EItemIconBackground::Transparent;
    FColor SolidColor8 = FColor(32, 32, 32, 255);
    int32 StyleVersion = 1;
    // Optional future extensibility (e.g., materials/attachments)
    uint32 MeshMaterialHash = 0;

    bool operator==(const FItemIconKey& Other) const
    {
        return ItemGuid == Other.ItemGuid
            && Resolution == Other.Resolution
            && Background == Other.Background
            && SolidColor8 == Other.SolidColor8
            && StyleVersion == Other.StyleVersion
            && MeshMaterialHash == Other.MeshMaterialHash;
    }
};

FORCEINLINE uint32 GetTypeHash(const FItemIconKey& Key)
{
    uint32 Hash = GetTypeHash(Key.ItemGuid);
    Hash = HashCombine(Hash, ::GetTypeHash(Key.Resolution));
    Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(Key.Background)));
    Hash = HashCombine(Hash, ::GetTypeHash(Key.SolidColor8.DWColor()));
    Hash = HashCombine(Hash, ::GetTypeHash(Key.StyleVersion));
    Hash = HashCombine(Hash, ::GetTypeHash(Key.MeshMaterialHash));
    return Hash;
}
