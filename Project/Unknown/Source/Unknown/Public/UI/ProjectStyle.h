#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SWidget.h"

struct FSlateBrush;
struct FButtonStyle;

namespace ProjectStyle
{
    // Returns a Slate font info configured to use the project's mono font at the given size.
    // Asset: /Game/ThirdParty/Fonts/Font_IBMPlexMono.Font_IBMPlexMono
    UNKNOWN_API FSlateFontInfo GetMonoFont(int32 Size);

    // Terminal-style UI helpers (black background, white borders/text)
    
    // Returns a terminal-style white border brush (for outlines)
    UNKNOWN_API const FSlateBrush* GetTerminalWhiteBrush();
    
    // Returns a terminal-style black background brush (for fills)
    UNKNOWN_API const FSlateBrush* GetTerminalBlackBrush();
    
    // Returns a terminal-style button style (black background, white text, no hover effects)
    UNKNOWN_API const FButtonStyle* GetTerminalButtonStyle();
    
    // Terminal color constants
    UNKNOWN_API FLinearColor GetTerminalWhite();
    UNKNOWN_API FLinearColor GetTerminalBlack();
    
    // Creates a terminal-styled menu row widget with hover accent
    // Label: Text to display
    // OnClick: Callback when clicked (will dismiss menus and restore focus)
    // Returns: Shared widget reference for the styled row
    UNKNOWN_API TSharedRef<SWidget> MakeTerminalMenuRow(const FString& Label, TFunction<void()> OnClick);
    
    // Creates a terminal-styled menu frame (white border, black background)
    // Content: Widget to wrap
    // Returns: Shared widget reference for the framed menu
    UNKNOWN_API TSharedRef<SWidget> MakeTerminalMenuFrame(TSharedRef<SWidget> Content);
}
