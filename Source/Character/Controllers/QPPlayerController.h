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
	//Pickup Target Setting Function
	UFUNCTION(BlueprintCallable, Category="UI|Pickup")
	void SetPickupTarget(AActor* NewTarget); // 픽업 타겟 설정 함수

protected:
	//Pickup Widget Class
	UPROPERTY(EditDefaultsOnly, Category = "UI|Pickup")
	TSubclassOf<class UQPPickupWidget> PickupWidgetClass; // 픽업 위젯 클래스
private:
	//Pickup Widget Instance
	UPROPERTY()
	TObjectPtr<class UQPPickupWidget> PickupWidget; // 픽업 위젯 인스턴스
};
