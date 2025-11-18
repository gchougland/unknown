#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SBoxPanel.h"
#include "SItemPlacerWidget.h"

static const FName ItemPlacerTabName(TEXT("ItemPlacerTab"));

class FUnknownEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        // Register tab spawner
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ItemPlacerTabName,
            FOnSpawnTab::CreateRaw(this, &FUnknownEditorModule::SpawnItemPlacerTab))
            .SetDisplayName(NSLOCTEXT("UnknownEditor", "ItemPlacerTabTitle", "Item Placer"))
            .SetTooltipText(NSLOCTEXT("UnknownEditor", "ItemPlacerTabTooltip", "Place item pickups from ItemDefinitions"))
            .SetMenuType(ETabSpawnerMenuType::Enabled);

        // Add menu entry under Window
        {
            UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
            if (Menu)
            {
                FToolMenuSection& Section = Menu->AddSection("UnknownEditor", NSLOCTEXT("UnknownEditor", "UnknownSection", "Unknown"));
                Section.AddMenuEntry(
                    "OpenItemPlacerTab",
                    NSLOCTEXT("UnknownEditor", "OpenItemPlacer", "Item Placer"),
                    NSLOCTEXT("UnknownEditor", "OpenItemPlacer_TT", "Open the Item Placer tab to spawn pickups"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateRaw(this, &FUnknownEditorModule::OpenItemPlacerTab))
                );
            }
        }
    }

    virtual void ShutdownModule() override
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ItemPlacerTabName);
    }

private:
    void OpenItemPlacerTab()
    {
        FGlobalTabmanager::Get()->TryInvokeTab(ItemPlacerTabName);
    }

    TSharedRef<SDockTab> SpawnItemPlacerTab(const FSpawnTabArgs& Args)
    {
        return SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            [
                SNew(SItemPlacerWidget)
            ];
    }
};

IMPLEMENT_MODULE(FUnknownEditorModule, UnknownEditor);