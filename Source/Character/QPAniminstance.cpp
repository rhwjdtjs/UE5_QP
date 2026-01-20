// Fill out your copyright notice in the Description page of Project Settings.


#include "QPAniminstance.h"
#include "QPCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"

void UQPAniminstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* Pawn = TryGetPawnOwner();
	CachedCharacter = Cast<AQPCharacter>(Pawn);
}

void UQPAniminstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!CachedCharacter.IsValid())
	{
		APawn* Pawn = TryGetPawnOwner();
		CachedCharacter = Cast<AQPCharacter>(Pawn);
	}
	AQPCharacter* Character = CachedCharacter.Get();
	if (!Character) return;
	Speed = Character->GetVelocity().Size2D(); //수평 속도 계산
	bIsCrouched = Character->bIsCrouched; //앉아있는지 여부 업데이트
	bIsSprinting = Character->IsSprinting(); //달리고 있는지 여부 업데이트
	if (Speed > 0.f) //움직임 방향 계산
	{
		const FVector Velocity = Character->GetVelocity();
		const FRotator ActorRotation = Character->GetActorRotation();

		// UE 기본 함수: -180 ~ 180 범위의 방향 값 반환
		Direction = CalculateDirection(Velocity, ActorRotation);
	}
	else
	{
		// 정지 상태에서는 방향을 0으로 고정
		// → 제자리 회전 시 다리 애니메이션이 섞이는 현상 방지
		Direction = 0.f;
	}

	if (UCharacterMovementComponent* MoveComponent = Character->GetCharacterMovement())
	{
		bIsInAir = MoveComponent->IsFalling(); //공중에 있는지 여부 업데이트
		bIsAccelerating = MoveComponent->GetCurrentAcceleration().SizeSquared() > 0.f; //가속 중인지 여부 업데이트
	}
	else {
		bIsInAir = false; //공중에 있는지 여부 업데이트
		bIsAccelerating = false; //가속 중인지 여부 업데이트
	}
	if (UQPCombatComponent* CombatComponent = Character->GetCombatComponent())
	{
		WeaponType = CombatComponent->GetEquippedWeaponType(); //장착된 무기 타입 업데이트
		bIsAttacking = CombatComponent->IsAttacking(); //공격 중인지 여부 업데이트
	}
	else
	{
		WeaponType = EQPWeaponType::EWT_None; //장착된 무기 타입 업데이트
		bIsAttacking = false; //공격 중인지 여부 업데이트
	}
}
