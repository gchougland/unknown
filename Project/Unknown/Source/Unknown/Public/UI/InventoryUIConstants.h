#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"

namespace InventoryUIConstants
{
    // Icon sizes
    static constexpr float IconSize_InfoPanel = 64.f;
    static constexpr float IconSize_ItemEntry = 48.f;
    static constexpr float IconSize_EquipmentSlot = 36.f;
    static const inline FVector2D IconSize_InfoPanel_Vec = FVector2D(64.f, 64.f);
    static const inline FVector2D IconSize_ItemEntry_Vec = FVector2D(48.f, 48.f);
    static const inline FVector2D IconSize_EquipmentSlot_Vec = FVector2D(36.f, 36.f);

    // Widget dimensions
    static constexpr float InventoryScreen_Width = 800.f;
    static constexpr float InventoryScreen_Height = 480.f;

    // Font sizes
    static constexpr int32 FontSize_Title = 28;
    static constexpr int32 FontSize_InfoName = 18;
    static constexpr int32 FontSize_InfoDesc = 12;
    static constexpr int32 FontSize_VolumeReadout = 14;
    static constexpr int32 FontSize_ItemEntry = 14;
    static constexpr int32 FontSize_Header = 12;
    static constexpr int32 FontSize_Hotkey = 14;
    static constexpr int32 FontSize_Quantity = 16;
    static constexpr int32 FontSize_EquipmentSlot = 12;

    // Padding values
    static constexpr float Padding_Outer = 2.f;
    static constexpr float Padding_Inner = 16.f;
    static constexpr float Padding_InnerSmall = 8.f;
    static constexpr float Padding_InnerTiny = 6.f;
    static constexpr float Padding_Cell = 1.f;
    static constexpr float Padding_TopBar = 12.f;
    static constexpr float Padding_InfoPanel = 8.f;
    static constexpr float Padding_Header = 6.f;
    static constexpr float Padding_ItemEntry = 2.f;
    static constexpr float Padding_EquipmentSlot = 2.f;
    static constexpr float Padding_Hotkey = 2.f;
    static constexpr float Padding_Quantity = 2.f;

    // Column widths (inventory list)
    static constexpr float ColumnWidth_Icon = 56.f;
    static constexpr float ColumnWidth_Name = 420.f;
    static constexpr float ColumnWidth_Quantity = 80.f;
    static constexpr float ColumnWidth_Mass = 100.f;
    static constexpr float ColumnWidth_Volume = 100.f;

    // Row heights
    static constexpr float RowHeight_Header = 28.f;
    static constexpr float RowHeight_ItemEntry = 52.f;

    // Hotbar slot size
    static constexpr float HotbarSlot_Size = 64.f; // Assuming square slots
    static const inline FVector2D HotbarSlot_Size_Vec = FVector2D(64.f, 64.f);

    // Shadow settings
    static const inline FVector2D ShadowOffset = FVector2D(1.f, 1.f);
    static constexpr float ShadowOpacity = 0.85f;
}

