#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LootListWidget.generated.h"


UCLASS()
class PJ_QUIET_PROTOCOL_API ULootListWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UScrollBox> LootScroll; // 전리품 스크롤 박스

	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<class ULootListEntryWidget> EntryWidgetClass; // 전리품 항목 위젯 클래스

	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	float ScanRadius = 300.f; // 전리품 검색 반경

	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	float RefreshInterval = 0.25f; // 전리품 목록 갱신 간격(초)

private:
	FTimerHandle RefreshTimer;

	void RefreshLootList(); // 전리품 목록 갱신 함수
};
