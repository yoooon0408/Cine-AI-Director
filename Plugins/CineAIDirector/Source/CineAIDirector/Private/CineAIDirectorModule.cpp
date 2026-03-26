// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "CineAIDirectorModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FCineAIDirectorModule"

void FCineAIDirectorModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("[CineAIDirector] 플러그인 시작"));
}

void FCineAIDirectorModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("[CineAIDirector] 플러그인 종료"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCineAIDirectorModule, CineAIDirector)
