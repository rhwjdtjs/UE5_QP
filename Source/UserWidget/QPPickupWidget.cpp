#include "QPPickupWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
void UQPPickupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime); // 부모 클래스의 틱 함수 호출
	if (!IsValid(TargetActor)) {
		TargetActor = nullptr; // 유효하지 않은 타겟은 nullptr로 설정
		SetVisibility(ESlateVisibility::Hidden); // 위젯 숨기기
		return;
	}
	APlayerController* PlayerController = GetOwningPlayer(); //플레이어 컨트롤러 가져오기
	if (!PlayerController) return;// 소유한 플레이어 컨트롤러 가져오기
	FVector2D ScreenPosition; // 화면 위치 변수
	if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, TargetActor->GetActorLocation(), ScreenPosition, true)) { // 월드 위치를 위젯 위치로 변환
		SetPositionInViewport(ScreenPosition, true); // 화면 위치에 위젯 배치
	}
}
