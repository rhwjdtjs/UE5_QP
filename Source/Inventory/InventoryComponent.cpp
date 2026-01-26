#include "InventoryComponent.h"
#include "ItemDataAsset.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UInventoryComponent::AddItem(UItemDataAsset* ItemData, int32 Quantity)
{
	if (!ItemData) return false; //유효한 아이템 데이터인지 확인
	if (Quantity <= 0) return false; //유효한 수량인지 확인

	for (int32 x = 0; x < Width; ++x) //인벤토리 가로 탐색
	{
		for (int32 y = 0; y < Height; ++y) //인벤토리 세로 탐색
		{
			const FIntPoint Position(x, y); //현재 위치 설정
			if(AddItemAt(ItemData, Quantity, Position)) //해당 위치에 아이템 추가 시도
			{
				UE_LOG(LogTemp, Warning, TEXT("[INV] AddItem OK: Slots=%d"), Slots.Num()); //디버그 로그 출력
				return true; //아이템 추가 성공
			}
		}
	}
	return false; //아이템 추가 실패
}
bool UInventoryComponent::AddItemAt(UItemDataAsset* ItemData, int32 Quantity, const FIntPoint& Position)
{
	if (!ItemData) return false; //유효한 아이템 데이터인지 확인
	if (Quantity <= 0) return false; //유효한 수량인지 확인
	if (!CanPlaceItemAt(ItemData, Position)) return false; //해당 위치에 아이템 배치가 가능한지 확인

	FInventorySlot NewSlot; //새 슬롯 생성
	NewSlot.Position = Position; //슬롯 위치 설정
	NewSlot.Item.ItemData = ItemData; //아이템 데이터 설정
	NewSlot.Item.Quantity = Quantity; //아이템 수량 설정
	Slots.Add(NewSlot); //슬롯을 인벤토리에 추가
	OnInventoryChanged.Broadcast(); //인벤토리 변경 알림 브로드캐스트
	return true; //아이템 추가 성공
}

bool UInventoryComponent::RemoveItemAt(const FIntPoint& Position)
{
	for (int32 i = 0; i < Slots.Num(); ++i) //인벤토리 슬롯 순회
	{
		if (Slots[i].Position == Position) //해당 위치의 슬롯 좌표가 일치하다면
		{
			Slots.RemoveAt(i); //슬롯 제거
			OnInventoryChanged.Broadcast(); //인벤토리 변경 알림 브로드캐스트
			return true; //아이템 제거 성공
		}
	}
	return false; //아이템 제거 실패
}

bool UInventoryComponent::MoveItem(const FIntPoint& From, const FIntPoint& To)
{
	if (From == To) return true; //같은 위치로 이동 시도 시 성공 처리

	const int32 FoundIndex = Slots.IndexOfByPredicate([&](const FInventorySlot& S) //인벤토리 슬롯에서 이동할 슬롯 찾기
		{
			return S.Position == From; //해당 위치의 슬롯 좌표가 일치하다면
		});

	if (FoundIndex == INDEX_NONE) //해당 위치에 아이템이 없다면
	{
		return false; //이동 실패
	}

	FInventorySlot MovingSlot = Slots[FoundIndex]; //이동할 슬롯 복사
	Slots.RemoveAt(FoundIndex); //이동할 슬롯 인벤토리에서 제거

	if (!CanPlaceItemAt(MovingSlot.Item.ItemData, To)) //새 위치에 아이템 배치가 가능한지 확인
	{
		Slots.Insert(MovingSlot, FoundIndex); // 원래 위치로 복원
		return false; //이동 실패
	}

	MovingSlot.Position = To; //슬롯 위치 업데이트
	Slots.Add(MovingSlot);//업데이트된 슬롯을 인벤토리에 추가
	OnInventoryChanged.Broadcast(); //인벤토리 변경 알림 브로드캐스트
	return true; //이동 성공
}

bool UInventoryComponent::CanPlaceItemAt(UItemDataAsset* ItemData, const FIntPoint& Position)
{
	if (!ItemData) return false; //유효한 아이템 데이터인지 확인

	FIntPoint ItemSize = ItemData->ItemSize; //아이템 크기 가져오기
	ItemSize.X = FMath::Max(1, ItemData->ItemSize.X); //아이템 가로 크기 최소값 설정
	ItemSize.Y = FMath::Max(1, ItemData->ItemSize.Y); //아이템 세로 크기 최소값 설정

	for (int32 x = 0; x < ItemSize.X; ++x) //아이템 가로 크기만큼 반복
	{
		for (int32 y = 0; y < ItemSize.Y; ++y) //아이템 세로 크기만큼 반복
		{
			if (!IsWithinBounds(Position + FIntPoint(x, y))) //인벤토리 범위 내에 있는지 확인
			{
				return false; //범위 벗어남
			}
		}
	}
	if(IsOverlapping(ItemData, Position)) //다른 아이템과 겹치는지 확인
	{
		return false; //겹침
	}
	return true; //아이템 배치 가능
}

bool UInventoryComponent::FindSlotAt(const FIntPoint& Position, FInventorySlot& OutSlot) const
{
	for (const FInventorySlot& Slot : Slots) //인벤토리 슬롯 순회
	{
		if (Slot.Position == Position) //해당 위치의 슬롯 좌표가 일치하다면
		{
			OutSlot = Slot; //출력 슬롯에 해당 슬롯 정보 복사
			return true; //슬롯 반환
		}
	}
	return false; //슬롯을 찾지 못함
}

bool UInventoryComponent::FindSlotContaining(const FIntPoint& Cell, FInventorySlot& Outslot) const
{
	for (const FInventorySlot& Slot : Slots)
	{
		if (!Slot.Item.ItemData) continue; //유효한 아이템 데이터인지 확인

		const FIntPoint SlotPos = Slot.Position; //슬롯 위치
		FIntPoint SlotSize = Slot.Item.ItemData->ItemSize; //슬롯 아이템 크기
		SlotSize.X = FMath::Max(1, SlotSize.X); //슬롯 아이템 가로 크기 최소값 설정
		SlotSize.Y = FMath::Max(1, SlotSize.Y); //슬롯 아이템 세로 크기 최소값 설정
		
		const bool bInX = (Cell.X >= SlotPos.X) && (Cell.X < SlotPos.X + SlotSize.X); //X축 포함 여부
		const bool bInY = (Cell.Y >= SlotPos.Y) && (Cell.Y < SlotPos.Y + SlotSize.Y); //Y축 포함 여부
		if (bInX && bInY)
		{
			Outslot = Slot; //출력 슬롯에 해당 슬롯 정보 복사
			return true; //슬롯 반환
		}
	}
	return false; //슬롯을 찾지 못함
}



bool UInventoryComponent::IsWithinBounds(const FIntPoint& Position) const
{
	return Position.X >= 0 && Position.Y >= 0 && Position.X < Width && Position.Y < Height;
}

bool UInventoryComponent::IsOverlapping(UItemDataAsset* ItemData, const FIntPoint& Position) const
{
	if (!ItemData) return false; //유효한 아이템 데이터인지 확인

	FIntPoint ItemSize = ItemData->ItemSize; //아이템 크기 가져오기
	ItemSize.X = FMath::Max(1, ItemData->ItemSize.X); //아이템 가로 크기 최소값 설정
	ItemSize.Y = FMath::Max(1, ItemData->ItemSize.Y); //아이템 세로 크기 최소값 설정

	for (const FInventorySlot& Slot : Slots) //인벤토리 슬롯 순회
	{
		if (!Slot.Item.ItemData) continue; //유효한 아이템 데이터인지 확인

		const FIntPoint ExistPos = Slot.Position; //기존 슬롯 위치
		FIntPoint ExistSize = Slot.Item.ItemData->ItemSize; //기존 아이템 크기
		ExistSize.X = FMath::Max(1, ExistSize.X); //기존 아이템 가로 크기 최소값 설정
		ExistSize.Y = FMath::Max(1, ExistSize.Y); //기존 아이템 세로 크기 최소값 설정

		//사격형 충돌 검사
		const bool bOverlapX = Position.X < ExistPos.X + ExistSize.X && Position.X + ItemSize.X > ExistPos.X; //X축 겹침 여부
		const bool bOverlapY = Position.Y < ExistPos.Y + ExistSize.Y && Position.Y + ItemSize.Y > ExistPos.Y; //Y축 겹침 여부
		if(bOverlapX && bOverlapY) //둘 다 겹친다면
		{
			return true; //겹침
		}
	}
	return false; //겹침 아님
}


