#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * 에디터 전용 모듈
 *
 * [언리얼 담당] 에디터 내 UI, 패널, 프리뷰 기능을 여기에 추가합니다.
 *               런타임 코드와 완전히 분리되어 있어서 패키징에 영향을 주지 않습니다.
 */
class FCineAIDirectorEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    // 에디터 메뉴/툴바 항목 등록
    void RegisterMenuExtensions();
    void UnregisterMenuExtensions();
};
