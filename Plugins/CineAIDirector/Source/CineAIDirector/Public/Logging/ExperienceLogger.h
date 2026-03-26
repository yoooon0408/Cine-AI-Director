#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SceneContext.h"
#include "Data/DirectorDecision.h"
#include "ExperienceLogger.generated.h"

/**
 * ★ UE5 ↔ ML 파이프라인의 다리 역할을 하는 핵심 컴포넌트 ★
 *
 * AI 디렉터의 모든 결정을 (State, Action, Reward) 형태로 기록합니다.
 * 이 로그가 ML/data/raw/ 로 흘러가서 강화학습 데이터가 됩니다.
 *
 * [언리얼 담당] 이 파일은 건드리지 않아도 됩니다.
 *               AIDirector가 자동으로 호출합니다.
 *
 * [ML 담당] 로그 포맷이나 저장 경로를 바꾸고 싶으면 .cpp를 수정하세요.
 *           리워드 업데이트는 UpdateReward() 함수를 사용합니다.
 */
UCLASS(ClassGroup=(CineAI), meta=(BlueprintSpawnableComponent))
class CINEAIDIRECTOR_API UExperienceLogger : public UActorComponent
{
    GENERATED_BODY()

public:
    UExperienceLogger();

    /**
     * (State=Context, Action=Decision, Reward=초기값) 을 기록합니다.
     * AIDirector가 결정을 내릴 때마다 자동 호출됩니다.
     *
     * @return 이 경험의 고유 ID (리워드 업데이트 시 사용)
     */
    FString LogExperience(const FSceneContext& Context, const FDirectorDecision& Decision, float InitialReward);

    /**
     * 특정 경험의 리워드를 사후에 업데이트합니다.
     * (예: 씬 종료 후 품질 평가가 끝났을 때)
     *
     * [ML 담당] 리워드 신호 설계는 RewardEvaluator.h에서 담당합니다.
     */
    void UpdateReward(const FString& ExperienceID, float Reward);

    // 로그 파일 저장 경로 (기본값: 프로젝트/Saved/CineAILogs/)
    UPROPERTY(EditAnywhere, Category="CineAI|Logging")
    FString LogDirectory = TEXT("");

    // 세션별로 파일을 분리할지 여부
    UPROPERTY(EditAnywhere, Category="CineAI|Logging")
    bool bSeparateBySession = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    FString CurrentSessionID;
    FString CurrentLogFilePath;

    // 리워드 업데이트 전용 파일 경로 (reward_updates_<session>.jsonl)
    FString RewardUpdateFilePath;

    TArray<TSharedPtr<FJsonObject>> PendingEntries;

    // UpdateReward()로 누적된 업데이트 맵 (ID → 최종 reward)
    // EndPlay 시 별도 파일에 플러시됩니다.
    TMap<FString, float> PendingRewardUpdates;

    void InitLogFile();
    void FlushToDisk();
    void FlushRewardUpdates();
    FString GenerateID() const;
};
