#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

namespace ProjectStyle
{
    // Returns a Slate font info configured to use the project's mono font at the given size.
    // Asset: /Game/ThirdParty/Fonts/Font_IBMPlexMono.Font_IBMPlexMono
    UNKNOWN_API FSlateFontInfo GetMonoFont(int32 Size);
}
