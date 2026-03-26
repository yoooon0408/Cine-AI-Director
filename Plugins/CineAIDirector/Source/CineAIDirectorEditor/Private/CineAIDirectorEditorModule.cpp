// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "CineAIDirectorEditorModule.h"

#define LOCTEXT_NAMESPACE "FCineAIDirectorEditorModule"

void FCineAIDirectorEditorModule::StartupModule()
{
    RegisterMenuExtensions();
    UE_LOG(LogTemp, Log, TEXT("[CineAIDirectorEditor] 에디터 모듈 시작"));
}

void FCineAIDirectorEditorModule::ShutdownModule()
{
    UnregisterMenuExtensions();
}

void FCineAIDirectorEditorModule::RegisterMenuExtensions()
{
    // TODO: [언리얼 담당] 에디터 메뉴 항목 추가 (예: 디렉터 패널 열기)
}

void FCineAIDirectorEditorModule::UnregisterMenuExtensions()
{
    // TODO: 등록된 메뉴 항목 제거
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCineAIDirectorEditorModule, CineAIDirectorEditor)
