// Fill out your copyright notice in the Description page of Project Settings.


#include "QPPlayerController.h"
#include "PJ_Quiet_Protocol/UserWidget/QPPickupWidget.h"
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
}

void AQPPlayerController::SetPickupTarget(AActor* NewTarget)
{
	if (!IsLocalController()) return; // 로컬 컨트롤러인지 확인
	PickupWidget->SetTargetActor(NewTarget); // 타겟 액터 설정
	const bool bShow = (NewTarget != nullptr);
	PickupWidget->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden); // 위젯 표시 여부 설정
}
