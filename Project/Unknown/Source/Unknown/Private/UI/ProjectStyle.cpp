#include "UI/ProjectStyle.h"
#include "Engine/AssetManager.h"
#include "UObject/SoftObjectPath.h"

namespace ProjectStyle
{
    static TSoftObjectPtr<UObject> GMonoFontAsset = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/ThirdParty/Fonts/Font_IBMPlexMono.Font_IBMPlexMono")));

    static UObject* LoadFontObject()
    {
        UObject* FontObj = GMonoFontAsset.Get();
        if (!FontObj)
        {
            if (GMonoFontAsset.IsNull())
            {
                GMonoFontAsset = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/ThirdParty/Fonts/Font_IBMPlexMono.Font_IBMPlexMono")));
            }
            if (UAssetManager* AM = UAssetManager::GetIfValid())
            {
                FontObj = AM->GetStreamableManager().LoadSynchronous(GMonoFontAsset.ToSoftObjectPath(), false);
            }
            else
            {
                FontObj = GMonoFontAsset.LoadSynchronous();
            }
        }
        return FontObj;
    }

    FSlateFontInfo GetMonoFont(int32 Size)
    {
        UObject* FontObj = LoadFontObject();
        FSlateFontInfo Info;
        Info.Size = Size;
        Info.FontObject = FontObj;
        // Some fonts expose face names like "Regular"; leaving default if unknown
        return Info;
    }
}
