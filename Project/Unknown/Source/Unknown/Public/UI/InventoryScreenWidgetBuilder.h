#pragma once

#include "CoreMinimal.h"

class UWidgetTree;
class UBorder;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UImage;
class USizeBox;
class UInventoryListWidget;
class UEquipmentPanelWidget;
class UEquipmentComponent;
class UInventoryScreenWidget;
struct FLinearColor;

namespace InventoryScreenWidgetBuilder
{
    // Forward declarations for widget references
    struct FWidgetReferences
    {
        UBorder* RootBorder = nullptr;
        UVerticalBox* RootVBox = nullptr;
        UTextBlock* TitleText = nullptr;
        UTextBlock* VolumeText = nullptr;
        UHorizontalBox* BodyHBox = nullptr;
        UVerticalBox* LeftColumnVBox = nullptr;
        UBorder* EquipOuter = nullptr;
        UEquipmentPanelWidget* EquipmentPanel = nullptr;
        UBorder* HeaderOuter = nullptr;
        UInventoryListWidget* InventoryList = nullptr;
        UBorder* InfoOuterBorder = nullptr;
        UBorder* InfoInnerBorder = nullptr;
        UImage* InfoIcon = nullptr;
        UTextBlock* InfoNameText = nullptr;
        UTextBlock* InfoDescText = nullptr;
    };

    /**
     * Builds the root layout structure (outer border, size box, root vertical box)
     */
    UNKNOWN_API void BuildRootLayout(
        UWidgetTree* WidgetTree,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        FWidgetReferences& OutRefs
    );

    /**
     * Builds the top bar with title and volume readout
     */
    UNKNOWN_API void BuildTopBar(
        UWidgetTree* WidgetTree,
        UVerticalBox* RootVBox,
        const FLinearColor& TextColor,
        FWidgetReferences& OutRefs
    );

    /**
     * Builds the body layout (horizontal box for left/right columns)
     */
    UNKNOWN_API void BuildBodyLayout(
        UWidgetTree* WidgetTree,
        UVerticalBox* RootVBox,
        FWidgetReferences& OutRefs
    );

    /**
     * Builds the inventory column (header, list, info panel)
     * Note: The caller is responsible for binding delegates to InventoryList
     */
    UNKNOWN_API void BuildInventoryColumn(
        UWidgetTree* WidgetTree,
        UVerticalBox* LeftColumnVBox,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        const FLinearColor& TextColor,
        FWidgetReferences& OutRefs
    );

    /**
     * Builds the equipment panel container
     */
    UNKNOWN_API void BuildEquipmentPanel(
        UWidgetTree* WidgetTree,
        UHorizontalBox* BodyHBox,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        UEquipmentComponent* Equipment,
        FWidgetReferences& OutRefs
    );

    /**
     * Builds the header row with column titles
     */
    UNKNOWN_API void BuildHeaderRow(
        UWidgetTree* WidgetTree,
        UVerticalBox* LeftColumnVBox,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        const FLinearColor& TextColor
    );

    /**
     * Builds the info panel below the inventory list
     */
    UNKNOWN_API void BuildInfoPanel(
        UWidgetTree* WidgetTree,
        UVerticalBox* LeftColumnVBox,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        const FLinearColor& TextColor,
        FWidgetReferences& OutRefs
    );
}

