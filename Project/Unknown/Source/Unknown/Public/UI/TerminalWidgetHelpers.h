#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"
#include "Styling/SlateColor.h"

class UWidgetTree;
class UBorder;
class UTextBlock;
class USizeBox;
class UHorizontalBox;
class UVerticalBox;
class UImage;
class UScrollBox;
class UEditableTextBox;
class UWidget;

namespace TerminalWidgetHelpers
{
    // Creates a terminal-styled border widget (outer white border, inner black background)
    // WidgetTree: The widget tree to construct widgets in
    // OuterBorderColor: Color for the outer border (typically white)
    // InnerBackgroundColor: Color for the inner background (typically black)
    // OuterPadding: Padding for the outer border
    // InnerPadding: Padding for the inner background
    // OuterName: Optional name for the outer border widget
    // InnerName: Optional name for the inner border widget
    // Returns: The outer border widget (inner border is set as its content)
    UNKNOWN_API UBorder* CreateTerminalBorderedPanel(
        UWidgetTree* WidgetTree,
        const FLinearColor& OuterBorderColor,
        const FLinearColor& InnerBackgroundColor,
        const FMargin& OuterPadding = FMargin(2.f),
        const FMargin& InnerPadding = FMargin(16.f),
        const FName& OuterName = NAME_None,
        const FName& InnerName = NAME_None
    );

    // Creates a terminal-styled text block
    // WidgetTree: The widget tree to construct widgets in
    // Text: The text to display
    // FontSize: Font size for the mono font
    // TextColor: Color for the text (typically white)
    // Name: Optional name for the text block widget
    // Returns: The text block widget
    UNKNOWN_API UTextBlock* CreateTerminalTextBlock(
        UWidgetTree* WidgetTree,
        const FString& Text,
        int32 FontSize,
        const FLinearColor& TextColor,
        const FName& Name = NAME_None
    );

    // Creates a header cell for table headers
    // WidgetTree: The widget tree to construct widgets in
    // ParentHBox: The horizontal box to add the cell to
    // Width: Width of the cell
    // Label: Text label for the cell
    // BorderColor: Color for the border (typically white)
    // BackgroundColor: Color for the background (typically black)
    // TextColor: Color for the text (typically white)
    // Returns: The cell border widget
    UNKNOWN_API UBorder* CreateTerminalHeaderCell(
        UWidgetTree* WidgetTree,
        UHorizontalBox* ParentHBox,
        float Width,
        const FString& Label,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        const FLinearColor& TextColor
    );

    // Creates a terminal-styled scroll box with black background and white border
    // WidgetTree: The widget tree to construct widgets in
    // OuterPadding: Padding for the outer border (default 2px)
    // InnerPadding: Padding for the inner background (default 8px)
    // Name: Optional name for the scroll box widget
    // Returns: The scroll box widget wrapped in terminal-styled border panel
    UNKNOWN_API UScrollBox* CreateTerminalScrollBox(
        UWidgetTree* WidgetTree,
        const FMargin& OuterPadding = FMargin(2.f),
        const FMargin& InnerPadding = FMargin(8.f),
        const FName& Name = NAME_None
    );

    // Creates a terminal-styled text input field
    // WidgetTree: The widget tree to construct widgets in
    // PlaceholderText: Placeholder text to display when empty
    // FontSize: Font size for the mono font (default 18)
    // Width: Optional width override
    // Height: Optional height override
    // Name: Optional name for the text box widget
    // Returns: The editable text box widget with terminal styling
    UNKNOWN_API UEditableTextBox* CreateTerminalEditableTextBox(
        UWidgetTree* WidgetTree,
        const FString& PlaceholderText = TEXT(""),
        int32 FontSize = 18,
        float Width = 0.f,
        float Height = 0.f,
        const FName& Name = NAME_None
    );

    // Creates a terminal-styled modal dialog frame structure
    // WidgetTree: The widget tree to construct widgets in
    // Content: Widget to wrap in the dialog frame
    // Width: Optional width for the dialog (0 = auto)
    // Height: Optional height for the dialog (0 = auto)
    // OuterPadding: Padding for the outer border (default 2px)
    // InnerPadding: Padding for the inner background (default 16px)
    // Name: Optional name for the dialog widget
    // Returns: The outer border widget (inner border is set as its content)
    UNKNOWN_API UBorder* CreateTerminalModalDialog(
        UWidgetTree* WidgetTree,
        UWidget* Content,
        float Width = 0.f,
        float Height = 0.f,
        const FMargin& OuterPadding = FMargin(2.f),
        const FMargin& InnerPadding = FMargin(16.f),
        const FName& Name = NAME_None
    );

}

