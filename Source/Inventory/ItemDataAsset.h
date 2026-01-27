#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryHeaders/ItemTypes.h"
#include "ItemDataAsset.generated.h"


UCLASS(BlueprintType) //블루프린트에서 사용할 수 있는 데이터 에셋 클래스
class PJ_QUIET_PROTOCOL_API UItemDataAsset : public UDataAsset
{
	GENERATED_BODY() //UE 리플렉션 코드 생성 매크로

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Name", ToolTip="아이템 이름 기입"))
	FText ItemName; //아이템 이름

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Description", ToolTip = "아이템에 대한 설명 기입"))
	FText ItemDescription; //아이템 설명

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Icon", ToolTip = "아이템 아이콘 설정"))
	UTexture2D* ItemIcon; //아이템 아이콘

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Type", ToolTip = "아이템 타입 설정"))
	EItemType ItemType; //아이템 타입

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (DisplayName = "Item Grid Size", ToolTip = "인벤토리 그리드 크기 조정"))
	FIntPoint ItemSize; //아이템 그리드 크기 예:(2,3) 크기
	//FintPoint =  언리얼 엔진의 2D 정수 좌표/크기를 표현하는 구조체 
	//기본 구조 int32 X(가로 칸수); int32 Y(세로 칸수) ;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (DisplayName = "Weapon"))
	TSubclassOf<class AWeaponBase> WeaponClass; //무기 아이템일 경우 무기 클래스 참조
};
