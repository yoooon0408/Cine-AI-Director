// Copyright Cine-AI-Director Team. All Rights Reserved.

using UnrealBuildTool;

public class CineAIDirectorEditor : ModuleRules
{
    public CineAIDirectorEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "CineAIDirector",    // 런타임 모듈 참조
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UnrealEd",
            "Slate",
            "SlateCore",
            "EditorStyle",
            "PropertyEditor",
            "WorkspaceMenuStructure",
        });
    }
}
