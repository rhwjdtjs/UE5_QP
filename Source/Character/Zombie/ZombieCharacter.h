
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"

UCLASS()
class PJ_QUIET_PROTOCOL_API AZombieCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZombieCharacter();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Stat")
	float WalkSpeed = 150.f; //좀비 걷기 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Stat")
	float ChaseSpeed = 300.f; //좀비 추격 속도
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Stat")
	float AttackPower = 10.f; //좀비 공격력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Sense")
	float DetectRange = 1200.f; //플레이어 감지 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Combat")
	float AttackRange = 150.f; //좀비 공격 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Combat")
	float AttackCoolDown = 1.2f; //좀비 공격 쿨타임
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zombie|Combat")
	UAnimMontage* AttackMontage = nullptr; //좀비 공격 모션

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Runtime")
	TObjectPtr<AActor> TargetActor = nullptr; //현재 타겟팅된 액터
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Runtime")
	bool bIsAttacking = false; //공격 중인지 여부

	UFUNCTION(BlueprintCallable, Category = "Zombie")
	void SetTarget(AActor* NewTarget); //타겟 설정 함수
	UFUNCTION(BlueprintCallable, Category = "Zombie")
	bool CanAttackTarget() const;//타겟 공격 가능 여부 확인 함수
	UFUNCTION(BlueprintCallable, Category = "Zombie")
	void StartAttack(); //공격 시작 함수
private:
	float LastAttackTime = -1000.f; //마지막 공격 시간

};
