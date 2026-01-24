#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemDataAsset.h"
#include "WorldItemActor.generated.h"

UCLASS()
class PJ_QUIET_PROTOCOL_API AWorldItemActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AWorldItemActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	UItemDataAsset* ItemData; //아이템 정보

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 Quantity; //아이템 수량

};
