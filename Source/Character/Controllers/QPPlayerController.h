// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "QPPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override; 
	//Pickup Target Setting Function
	UFUNCTION(BlueprintCallable, Category="UI|Pickup")
	void SetPickupTarget(AActor* NewTarget); // 픽업 타겟 설정 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	void ToggleInventory(); // 인벤토리 토글 함수

protected:
	//Pickup Widget Class
	UPROPERTY(EditDefaultsOnly, Category = "UI|Pickup")
	TSubclassOf<class UQPPickupWidget> PickupWidgetClass; // 픽업 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|Inventory")
	TSubclassOf<class UUserWidget> InventoryWidgetClass; // 인벤토리 위젯 클래스

private:
	//Pickup Widget Instance
	UPROPERTY()
	TObjectPtr<class UQPPickupWidget> PickupWidget; // 픽업 위젯 인스턴스
	UPROPERTY()
	TObjectPtr<class UUserWidget> InventoryWidget; // 인벤토리 위젯 인스턴스

	bool bInventoryOpen = false; // 인벤토리 열림 여부
	void SetInventoryOpen(bool bOpen); // 인벤토리 열림 상태 설정 함수
};
