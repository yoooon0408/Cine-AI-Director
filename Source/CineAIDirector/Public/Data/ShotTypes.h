#pragma once

#include "CoreMinimal.h"
#include "ShotTypes.generated.h"

/**
 * 씬 상황 분류
 * [언리얼 담당] 게임 상황에 맞게 enum 값을 추가/수정해도 됩니다.
 *               단, 추가하면 ML 담당자에게 꼭 알려주세요! (reward_model.py에 반영 필요)
 */
UENUM(BlueprintType)
enum class ECineSceneType : uint8
{
    Unknown         UMETA(DisplayName = "알 수 없음"),
    Dialogue        UMETA(DisplayName = "대화"),
    Combat          UMETA(DisplayName = "전투"),
    Exploration     UMETA(DisplayName = "탐험/이동"),
    Cutscene        UMETA(DisplayName = "컷씬"),
    Death           UMETA(DisplayName = "사망"),
    Victory         UMETA(DisplayName = "승리"),
};

/**
 * 카메라 샷 타입
 * [언리얼 담당] 이 enum이 카메라가 실제로 취할 수 있는 샷의 목록입니다.
 * [ML 담당] RL 에이전트의 Action Space가 이 enum 기반입니다.
 */
UENUM(BlueprintType)
enum class ECineShotType : uint8
{
    None                UMETA(DisplayName = "없음"),
    ExtremeWide         UMETA(DisplayName = "익스트림 와이드"),
    Wide                UMETA(DisplayName = "와이드"),
    Medium              UMETA(DisplayName = "미디엄"),
    MediumCloseUp       UMETA(DisplayName = "미디엄 클로즈업"),
    CloseUp             UMETA(DisplayName = "클로즈업"),
    ExtremeCloseUp      UMETA(DisplayName = "익스트림 클로즈업"),
    OverShoulder        UMETA(DisplayName = "오버숄더"),
    TwoShot             UMETA(DisplayName = "투샷"),
    POV                 UMETA(DisplayName = "POV (시점 샷)"),
    BirdsEye            UMETA(DisplayName = "버즈아이 (항공 시점)"),
    LowAngle            UMETA(DisplayName = "로우앵글"),
    Dutch               UMETA(DisplayName = "더치앵글 (기울임)"),
};

/**
 * 카메라 트랜지션 방식
 * [언리얼 담당] ShotBlender에서 실제 블렌딩 구현 시 참고하세요.
 */
UENUM(BlueprintType)
enum class ECineTransitionType : uint8
{
    Cut         UMETA(DisplayName = "컷 (즉시 전환)"),
    Blend       UMETA(DisplayName = "블렌드 (부드럽게)"),
    Fade        UMETA(DisplayName = "페이드"),
};
