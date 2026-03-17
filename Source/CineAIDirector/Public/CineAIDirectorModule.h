# pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * CineAIDirector 플러그인의 메인 모듈
 * 플러그인 시작/종료 시 초기화 작업을 담당합니다.
 *
 * [언리얼 담당] 이 파일은 거의 건드릴 일 없어요.
 *               플러그인 로드/언로드 시 자동으로 호출됩니다.
 */
class FCineAIDirectorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
