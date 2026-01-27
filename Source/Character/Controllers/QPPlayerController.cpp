// Fill out your copyright notice in the Description page of Project Settings.


#include "QPPlayerController.h"
#include "PJ_Quiet_Protocol/UserWidget/QPPickupWidget.h"
#include "Blueprint/UserWidget.h"
void AQPPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return; // 로컬 컨트롤러인지 확인
	if (PickupWidgetClass) {
		PickupWidget = CreateWidget<UQPPickupWidget>(this, PickupWidgetClass); // 위젯 인스턴스 생성
		if(PickupWidget) {
			PickupWidget->AddToViewport(); // 뷰포트에 추가
			PickupWidget->SetTargetActor(nullptr); // 타겟 액터 초기화
			PickupWidget->SetVisibility(ESlateVisibility::Hidden); // 위젯 숨기기
		}
	}
	if (InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass); // 인벤토리 위젯 인스턴스 생성
		if (InventoryWidget) {
			InventoryWidget->AddToViewport(10); // 뷰포트에 추가
			InventoryWidget->SetVisibility(ESlateVisibility::Hidden); // 인벤토리 위젯 숨기기
		}
	}
}

void AQPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);
	InputComponent->BindAction("ToggleInventory", IE_Pressed, this, &AQPPlayerController::ToggleInventory);
}

void AQPPlayerController::SetPickupTarget(AActor* NewTarget)
{
	if (!IsLocalController()) return; // 로컬 컨트롤러인지 확인
	if (!PickupWidget) return; // 픽업 위젯이 유효한지 확인

	PickupWidget->SetTargetActor(NewTarget); // 타겟 액터 설정
	const bool bShow = (NewTarget != nullptr);
	PickupWidget->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden); // 위젯 표시 여부 설정
}

void AQPPlayerController::ToggleInventory()
{
	SetInventoryOpen(!bInventoryOpen); // 인벤토리 열림 상태 토글
}

void AQPPlayerController::SetInventoryOpen(bool bOpen)
{
	if (!IsLocalPlayerController()) return; // 로컬 플레이어 컨트롤러인지 확인

	bInventoryOpen = bOpen; // 인벤토리 열림 상태 설정

	if (!InventoryWidget && InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass); // 인벤토리 위젯 인스턴스 생성
		if (InventoryWidget) {
			InventoryWidget->AddToViewport(10); // 뷰포트에 추가
			InventoryWidget->SetVisibility(ESlateVisibility::Hidden); // 인벤토리 위젯 숨기기
		}
	}

	if (!InventoryWidget) return; // 인벤토리 위젯이 유효한지 확인

	if (bInventoryOpen) {
		InventoryWidget->SetVisibility(ESlateVisibility::Visible); // 인벤토리 위젯 표시
		SetShowMouseCursor(true); // 마우스 커서 표시
		bEnableClickEvents = true; // 클릭 이벤트 활성화

		//이동/시야 입력 차단
		SetIgnoreMoveInput(true); // 이동 입력 무시
		SetIgnoreLookInput(true); // 시야 입력 무시

		FInputModeGameAndUI InputMode; // 입력 모드 설정
		InputMode.SetHideCursorDuringCapture(false); // 커서 캡처 중 커서 숨기지 않음
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 마우스 잠금 동작 설정
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget()); // 위젯에 포커스 설정
		SetInputMode(InputMode); // 입력 모드 적용
	}
	else
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Hidden); // 인벤토리 위젯 숨기기
		SetShowMouseCursor(false); // 마우스 커서 숨기기
		bEnableClickEvents = false; // 클릭 이벤트 비활성화
		bEnableMouseOverEvents = false; // 마우스 오버 이벤트 비활성화
		//이동/시야 입력 허용
		SetIgnoreMoveInput(false); // 이동 입력 허용
		SetIgnoreLookInput(false); // 시야 입력 허용
		FInputModeGameOnly InputMode; // 게임 전용 입력 모드 설정
		SetInputMode(InputMode); // 입력 모드 적용
	}
}
