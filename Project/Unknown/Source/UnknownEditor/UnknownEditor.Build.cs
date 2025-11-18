using UnrealBuildTool;

public class UnknownEditor : ModuleRules
{
    public UnknownEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UnrealEd",
            "LevelEditor",
            "ContentBrowser",
            "AssetRegistry",
            "EditorStyle",
            "InputCore",
            "ApplicationCore",
            "Projects",
            "ToolMenus",
            "EditorSubsystem",
            "UMG",
            "Unknown" // runtime module
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "EditorFramework",
                "Kismet",
                "KismetCompiler",
                "PropertyEditor"
            });
        }
    }
}
