#include "WorldItemActor.h"

AWorldItemActor::AWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemData = nullptr; //아이템 정보 초기화
	Quantity = 1; //아이템 수량 초기화
}


