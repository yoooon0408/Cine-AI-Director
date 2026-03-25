// Copyright Cine-AI-Director Team. All Rights Reserved.

using UnrealBuildTool;

public class CineAIDirector : ModuleRules
{
    public CineAIDirector(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "CinematicCamera",   // UE5 시네마틱 카메라 모듈
            "HTTP",              // 외부 AI API 호출용
            "Json",              // JSON 직렬화 (API 요청/응답 + 로그 저장)
            "JsonUtilities",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
        });
    }
}
