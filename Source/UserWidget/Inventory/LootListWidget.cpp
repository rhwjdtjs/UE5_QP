#include "LootListWidget.h"
#include "Components/ScrollBox.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "LootListEntryWidget.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "PJ_Quiet_Protocol/Inventory/WorldItemActor.h"
#include "Engine/OverlapResult.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
void ULootListWidget::NativeConstruct()
{
	Super::NativeConstruct(); // 부모 클래스의 NativeConstruct 호출
	if (UWorld* World = GetWorld()) // 월드 객체 가져오기
	{
		World->GetTimerManager().SetTimer(
			RefreshTimer,
			this,
			&ULootListWidget::RefreshLootList,
			RefreshInterval,
			true
		); // 타이머 설정하여 주기적으로 전리품 목록 갱신
	}
	else
	{
	}
}

void ULootListWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimer); // 타이머 해제
	}
	Super::NativeDestruct(); // 부모 클래스의 NativeDestruct 호출
}

void ULootListWidget::RefreshLootList()
{
	const ESlateVisibility WidgetVisibility = GetVisibility(); // 위젯의 현재 가시성 상태 가져오기
	if(WidgetVisibility == ESlateVisibility::Collapsed || WidgetVisibility == ESlateVisibility::Hidden)
	{
		return; // 위젯이 숨겨져 있으면 갱신하지 않음
	}
	if (!LootScroll)
	{
		return;
	}
	if (!EntryWidgetClass)
	{
		return;
	}

	AQPCharacter* Character = Cast<AQPCharacter>(GetOwningPlayerPawn()); // 소유한 플레이어 폰을 AQPCharacter로 캐스팅
	if (!Character)
	{
		return;
	}

	UWorld* World = GetWorld(); // 월드 객체 가져오기
	if (!World)
	{
		return;
	}

	const FVector Center = Character->GetActorLocation(); // 캐릭터 위치를 중심으로 설정

	FCollisionObjectQueryParams ObjParams; // 충돌 객체 쿼리 매개변수 설정
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic); // 월드 다이내믹 객체 포함
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic); // 월드 스태틱 객체 포함
	ObjParams.AddObjectTypesToQuery(ECC_PhysicsBody); // 물리 바디 객체 포함

	FCollisionShape Sphere = FCollisionShape::MakeSphere(ScanRadius); // 스캔 반경을 이용해 구형 충돌체 생성

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LootScan), false, Character); // 충돌 쿼리 매개변수 설정
	Params.AddIgnoredActor(Character); // 캐릭터 자신은 무시

	TArray<FOverlapResult> Overlaps; // 겹침 결과 배열
	const bool bOverlapped = World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjParams, Sphere, Params);

	struct FLootRow // 전리품 행 구조체
	{
		TWeakObjectPtr<AActor> Actor; // 전리품 액터
		TObjectPtr<UItemDataAsset> ItemData = nullptr; // 아이템 데이터 에셋
		int32 Quantity = 1; // 수량
		float DistSq = 0.f; // 거리 제곱
	};

	TArray<FLootRow> Rows; // 전리품 행 배열
	Rows.Reserve(Overlaps.Num()); // 배열 용량 예약

	TSet<TWeakObjectPtr<AActor>> UniqueActors; // 중복 없는 액터 집합

	for (const FOverlapResult& R : Overlaps) // 겹침 결과 반복
	{
		AActor* A = R.GetActor(); // 액터 가져오기
		if (!A || UniqueActors.Contains(A)) continue; // 유효하지 않거나 중복된 액터는 건너뜀
		UniqueActors.Add(A); // 액터를 집합에 추가

		if (AWeaponBase* Weapon = Cast<AWeaponBase>(A))
		{
			if (Weapon->GetOwner() != nullptr) continue; // 소유자가 있으면 건너뜀

			UItemDataAsset* WeaponItemData = Weapon->GetWeaponItemData(); // 무기 아이템 데이터 가져오기
			if (!WeaponItemData) continue;

			FLootRow Row; // 전리품 행 생성
			Row.Actor = Weapon; // 액터 설정
			Row.ItemData = WeaponItemData; // 아이템 데이터 설정
			Row.Quantity = 1; // 수량 설정
			Row.DistSq = FVector::DistSquared(Center, Weapon->GetActorLocation()); // 거리 제곱 계산
			Rows.Add(Row); // 행을 배열에 추가
			continue;
		}

		// 월드 아이템 액터만 리스트에 띄움
		if (AWorldItemActor* WorldItem = Cast<AWorldItemActor>(A)) // A를 AWorldItemActor로 캐스팅
		{
			if (!WorldItem->ItemData || WorldItem->Quantity <= 0) continue; // 아이템 데이터가 없거나 수량이 0 이하인 경우 건너뜀

			FLootRow Row; // 전리품 행 생성
			Row.Actor = WorldItem; // 액터 설정
			Row.ItemData = WorldItem->ItemData; // 아이템 데이터 설정
			Row.Quantity = WorldItem->Quantity; // 수량 설정
			Row.DistSq = FVector::DistSquared(Center, WorldItem->GetActorLocation()); // 거리 제곱 계산
			Rows.Add(Row); // 행을 배열에 추가
		}
	}
	Rows.Sort([](const FLootRow& L, const FLootRow& R) { return L.DistSq < R.DistSq; }); // 거리 기준으로 정렬

	LootScroll->ClearChildren(); // 기존 자식 위젯 제거
	for (const FLootRow& Row : Rows) // 각 전리품 행에 대해
	{
		if (!Row.Actor.IsValid() || !Row.ItemData) continue; // 유효성 검사

		ULootListEntryWidget* Entry = CreateWidget<ULootListEntryWidget>(GetOwningPlayer(), EntryWidgetClass); // 엔트리 위젯 생성
		if (!Entry) continue; // 유효성 검사

		Entry->Setup(Row.Actor.Get(), Row.ItemData, Row.Quantity); // 엔트리 설정
		LootScroll->AddChild(Entry); // 스크롤 박스에 자식으로 추가
	}
}
