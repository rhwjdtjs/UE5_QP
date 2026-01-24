#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

UENUM(BlueprintType) //블루프린트에서도 사용 가능하도록 설정
enum class EItemType : uint8 //아이템 타입 열거형
{
	EIT_None UMETA(DisplayName = "None"), //없음
	EIT_Weapon UMETA(DisplayName = "Weapon"), //무기
	EIT_Ammo UMETA(DisplayName = "Ammo"), //탄약
	EIT_Consumable UMETA(DisplayName = "Consumable"), //소모품
	EIT_Material UMETA(DisplayName = "Material"), //재료
	EIT_Miscellaneous UMETA(DisplayName = "Miscellaneous"), //기타
	EIT_Quest UMETA(DisplayName = "Quest") //퀘스트 아이템
};
