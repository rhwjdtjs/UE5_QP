#pragma once

#include "CoreMinimal.h"
#include "QPCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EQPWeaponType : uint8 //무기 타입 열거형
{
	EWT_None UMETA(DisplayName = "None"), //무기 없음
	EWT_Melee UMETA(DisplayName = "Melee"), //근접 무기
	EWT_Rifle UMETA(DisplayName = "Rifle"), //소총
	EWT_Shotgun UMETA(DisplayName = "Shotgun"), //샷건
	EWT_Handgun UMETA(DisplayName = "Handgun"), //권총
};

