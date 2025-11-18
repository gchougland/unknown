#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Icons/ItemIconSubsystem.h"
#include "Icons/ItemIconSettings.h"
#include "Icons/ItemIconTypes.h"
#include "Inventory/ItemDefinition.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

// Helper to build expected filename identical to subsystem logic
static FString BuildExpectedFilenameForKey(const FItemIconKey& Key)
{
    const TCHAR* Bg = (Key.Background == EItemIconBackground::Transparent) ? TEXT("T") : TEXT("S");
    auto ColorToHex = [](const FColor& C) { return FString::Printf(TEXT("%02X%02X%02X%02X"), C.A, C.R, C.G, C.B); };
    const FString File = FString::Printf(TEXT("%s_v%d_%d_%s_%s.png"), *Key.ItemGuid.ToString(EGuidFormats::DigitsWithHyphens), Key.StyleVersion, Key.Resolution, Bg, *ColorToHex(Key.SolidColor8));
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ItemIcons"), File);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemIconsGenerateAndReload,
    "Project.Smoke.ItemIcons.GenerateAndReload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

// Simple latent command that waits until a boolean flag becomes true
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForBoolLatentCommand, bool*, bFlagPtr);
bool FWaitForBoolLatentCommand::Update()
{
    return bFlagPtr && *bFlagPtr;
}

bool FItemIconsGenerateAndReload::RunTest(const FString& Parameters)
{
    UItemIconSubsystem* Sys = UItemIconSubsystem::Get();
    TestNotNull(TEXT("ItemIconSubsystem available"), Sys);
    if (!Sys)
    {
        return false;
    }

    // Resolve Crowbar definition
    UItemDefinition* Def = Cast<UItemDefinition>(StaticLoadObject(UItemDefinition::StaticClass(), nullptr, TEXT("/Game/Data/Items/DA_Crowbar.DA_Crowbar")));
    TestNotNull(TEXT("DA_Crowbar loaded"), Def);
    if (!Def)
    {
        return false;
    }

    // Style from settings
    FItemIconStyle Style;
    if (const UItemIconSettings* Settings = UItemIconSettings::Get())
    {
        Style.Resolution = Settings->DefaultResolution;
        Style.Background = Settings->bTransparentBackground ? EItemIconBackground::Transparent : EItemIconBackground::SolidColor;
        Style.SolidColor = Settings->SolidBackgroundColor;
        Style.StyleVersion = Settings->StyleVersion;
    }

    // Build expected filename
    FItemIconKey Key;
    Key.ItemGuid = Def->Guid;
    Key.Resolution = Style.Resolution;
    Key.Background = Style.Background;
    Key.SolidColor8 = Style.SolidColor.ToFColor(true);
    Key.StyleVersion = Style.StyleVersion;
    const FString ExpectedPath = BuildExpectedFilenameForKey(Key);

    // Ensure file does not exist before
    if (FPaths::FileExists(ExpectedPath))
    {
        IFileManager::Get().Delete(*ExpectedPath, false, true);
    }

    // Request icon asynchronously
    UTexture2D* CallbackTex = nullptr;
    bool bCompleted = false;
    Sys->RequestIcon(Def, Style, FOnItemIconReady::CreateLambda([&](const UItemDefinition* ReadyDef, UTexture2D* ReadyTex)
    {
        CallbackTex = ReadyTex;
        bCompleted = true;
    }));

    ADD_LATENT_AUTOMATION_COMMAND(FWaitForBoolLatentCommand(&bCompleted));

    ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, CallbackTex, ExpectedPath, Sys, Def, Style]() mutable
    {
        this->TestNotNull(TEXT("Generated texture non-null"), CallbackTex);
        this->TestTrue(TEXT("Icon file saved to disk"), FPaths::FileExists(ExpectedPath));

        // Clear memory cache to force disk load path
        Sys->InvalidateAll(Style.StyleVersion);

        // Synchronous path should hit disk
        UTexture2D* DiskTex = Sys->GetIconIfReady(Def, Style);
        this->TestNotNull(TEXT("Disk-loaded texture non-null"), DiskTex);
        return true;
    }, 0.01f));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
