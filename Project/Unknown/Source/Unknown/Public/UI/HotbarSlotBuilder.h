#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"

class UWidgetTree;
class UVerticalBox;
class UBorder;
class UImage;
class UTextBlock;
class USizeBox;
class UOverlay;
struct FLinearColor;

namespace HotbarSlotBuilder
{
    // Forward declarations for slot widget references
    struct FSlotWidgets
    {
        UBorder* OuterBorder = nullptr;
        UBorder* InnerBorder = nullptr;
        USizeBox* SizeBox = nullptr;
        UOverlay* Overlay = nullptr;
        UImage* Icon = nullptr;
        UTextBlock* Hotkey = nullptr;
        UTextBlock* Quantity = nullptr;
    };

    /**
     * Builds a single hotbar slot widget structure
     * @param WidgetTree The widget tree to construct widgets in
     * @param RootBox The vertical box to add the slot to
     * @param SlotIndex The index of the slot (0-based)
     * @param BorderColor Color for the outer border
     * @param SlotBackground Color for the inner background
     * @param TextColor Color for text elements
     * @param SlotSize Size of the slot
     * @param OutWidgets Output structure containing references to created widgets
     */
    UNKNOWN_API void BuildSlotWidget(
        UWidgetTree* WidgetTree,
        UVerticalBox* RootBox,
        int32 SlotIndex,
        const FLinearColor& BorderColor,
        const FLinearColor& SlotBackground,
        const FLinearColor& TextColor,
        const FVector2D& SlotSize,
        FSlotWidgets& OutWidgets
    );
}

