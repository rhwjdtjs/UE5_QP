// Fill out your copyright notice in the Description page of Project Settings.


#include "QPAniminstance.h"
#include "QPCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h" 
#include "Components/SkeletalMeshComponent.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"
#include "Kismet/KismetMathLibrary.h"

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
	bIsAiming = Character->IsAiming(); //조준하고 있는지 여부 업데이트

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

	AO_Yaw = Character->GetAO_Yaw();
	AO_Pitch = Character->GetAO_Pitch();


	// ================= IK =================

	bUseLeftHandIK = false; // 기본적으로 IK 비활성화

	if (UQPCombatComponent* CombatComponent = Character->GetCombatComponent())	// 전투 컴포넌트 가져오기
	{
		AWeaponBase* EquippedWeapon = CombatComponent->GetEquippedWeapon(); // 장착된 무기 가져오기
		if (!EquippedWeapon) return; // 무기가 없으면 종료

		USkeletalMeshComponent* WeaponMesh = EquippedWeapon->GetWeaponMesh(); // 무기 메쉬 가져오기
		if (!WeaponMesh) return; // 무기 메쉬가 없으면 종료

		USkeletalMeshComponent* CharacterMesh = Character->GetMesh(); // 캐릭터 메쉬 가져오기
		if (!CharacterMesh) return; // 캐릭터 메쉬가 없으면 종료

		// 무기 왼손 소켓 월드 트랜스폼 가져오기
		const FTransform SocketWorldTransform = WeaponMesh->GetSocketTransform(TEXT("LeftHandSocket"), RTS_World);

		FVector OutPosition; // hand_r 기준 위치
		FRotator OutRotation; // hand_r 기준 회전

		// hand_r 기준 위치와 회전 계산
		CharacterMesh->TransformToBoneSpace(TEXT("hand_r"), SocketWorldTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);

		LeftHandIKTransform = FTransform(FQuat(OutRotation), OutPosition); // 왼손 IK 트랜스폼 설정

		bUseLeftHandIK = true; // IK 사용 활성화
	}

	// ================= Turn In Place =================

	const FRotator ControlRotation(0.f, Character->GetBaseAimRotation().Yaw, 0.f); // 컨트롤러의 회전 (Yaw만 사용)
	const FRotator ActorRotation = Character->GetActorRotation(); // 액터의 회전

	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);// 회전 차이 계산

	float TargetRootYawOffset = DeltaRot.Yaw; // 목표 RootYawOffset 값 설정

	const float ABSRootYawOffset = FMath::Abs(TargetRootYawOffset); // 절대값 계산

	if (ABSRootYawOffset > MaxTurnAngle) // 최대 회전 각도 초과 시
	{
		const float YawToSubtract = ABSRootYawOffset - MaxTurnAngle; // 초과한 각도 계산
		const float YawMultiplier = (TargetRootYawOffset > 0.f) ? 1.f : -1.f; // 방향에 따른 곱셈 값 설정

		TargetRootYawOffset -= YawToSubtract * YawMultiplier; // 최대 각도 내로 클램프
	}

	if (Speed < 5.f && !bIsInAir) // 거의 정지 상태이고 공중에 있지 않을 때
	{
		RootYawOffset = FMath::FInterpTo(RootYawOffset, TargetRootYawOffset, DeltaSeconds, 6.f); // RootYawOffset 보간
	}
	else // 움직이고 있거나 공중에 있을 때
	{
		RootYawOffset = 0.f; // RootYawOffset 초기화
	}

}
