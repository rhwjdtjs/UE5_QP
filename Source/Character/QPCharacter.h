#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PJ_Quiet_Protocol/Commons/QPCombatTypes.h"
#include "QPCharacter.generated.h"

class UQPCombatComponent; //전방 선언
class AWeaponBase; //전방 선언
UCLASS()
class PJ_QUIET_PROTOCOL_API AQPCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AQPCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void SetOverlappingWeapon(AWeaponBase* Weapon);
	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	FORCEINLINE AWeaponBase* GetOverlappingWeapon() const { return OverlappingWeapon; } //겹쳐진 무기 반환 함수
	UFUNCTION(BlueprintPure, Category = "Combat")
	FORCEINLINE FVector GetDesiredCameraOffset() const 
	{ 
		bool bIsHoldingGun = (Weapontype == EQPWeaponType::EWT_Rifle || Weapontype == EQPWeaponType::EWT_Shotgun || Weapontype == EQPWeaponType::EWT_Handgun);
		if (IsAiming() && bIsHoldingGun)
		{
			FVector Result = AimingCameraOffset;
			if (bIsCrouched) 
			{
				// 서있을 때와 앉았을 때의 높이 차이만큼 조준 시 높이도 낮춤
				float HeightDiff = StandingCameraOffset.Z - CrouchCameraPosOffset.Z;
				Result.Z -= HeightDiff;
			}
			return Result;
		}
		return bIsCrouched ? CrouchCameraPosOffset : StandingCameraOffset; 
	} //원하는 카메라 오프셋 반환
	UFUNCTION(BlueprintPure, Category = "Combat")
	FORCEINLINE UQPCombatComponent* GetCombatComponent() const { return CombatComponent; } //전투 컴포넌트 반환 함수
	UFUNCTION(BlueprintPure, Category = "Combat")
	FORCEINLINE EQPWeaponType GetWeaponType() const { return Weapontype; } //장착된 무기 타입 반환 함수
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsSprinting() const; //앞으로 달리기는 중인지 반환 함수
	bool IsAiming() const; //조준 중인지

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw;  } // 현재 애니메이션 오프셋의 Yaw 값을 반환
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch;  } // 현재 애니메이션 오프셋의 Pitch 값을 반환
	FORCEINLINE bool IsTurningInPlace() const { return bIsTurningInPlace; } // 제자리 회전 중인지 반환

	void PlayFireMontage(bool bAming); // 무기 발사 몽타주 재생 함수
	void PlayReloadMontage(); // 재장전 몽타주 재생 함수

protected:
	virtual void BeginPlay() override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override; //앉기 시작시 호출
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override; //일어서기 시작시 호출


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UQPCombatComponent* CombatComponent; //전투 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* CameraBoom; //3인칭 플레이를 위한 스프링암 

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FollowCamera; //플레이어를 따라다니는 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	EQPWeaponType Weapontype = EQPWeaponType::EWT_None; //장착된 무기 타입
	UFUNCTION()
	void HandleWeaponTypeChanged(EQPWeaponType NewWeaponType); //무기 타입 변경 핸들러
	
	UFUNCTION()
	void HandleAimStateChanged(bool bIsAiming); // [Fix] 조준 상태 변경 핸들러 (속도 동기화)

	//움직임 속도 변수들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 600.f; //걷기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 900.f; //달리기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CrouchSpeed = 300.f; //앉기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CrouchSprintSpeed = 700.f; //앉은 상태에서 달리기 속도

	FRotator StartingAimRotation; //시작 에임 회전 값

	//앉기 카메라 변수들
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	FVector StandingCameraOffset = FVector(0.f, 0.f, 120.f); //서있을 때 카메라 오프셋 (기본 높이 상향)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch")
	FVector CrouchCameraPosOffset = FVector(0.f, 0.f, 140.f); //앉아있을 때 카메라 오프셋 (CapsuleDrop 보정, 변수명 변경으로 초기화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch", meta = (ClampMin = "0.0"))
	float CrouchCameraInterpSpeed = 12.f; //카메라 위치 보간 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraArmLength = 400.f; //기본 카메라 거리 (300 -> 400 시야 확보)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float AimingArmLength = 200.f; //조준 시 카메라 거리 (150 -> 200 약간 뒤로)
	float DefaultArmLength; //기본 카메라 거리 (저장용)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch", meta = (ClampMin = "0.0"))
	float MinVerticalArmLength = 250.f; // 위를 볼 때 최소 거리 (줌인)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Crouch", meta = (ClampMin = "0.0"))
	float MaxVerticalArmLength = 600.f; // 아래를 볼 때 최대 거리 (줌아웃)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Aim")
	FVector AimingCameraOffset = FVector(0.f, 40.f, 80.f); //조준 시 카메라 오프셋 (Y=70->40: 왼쪽으로 이동(오른쪽 치우침 완화), Z=60: 높이 조정)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon", meta = (ClampMin = "0.0"))
	float EquipTraceDistance = 250.f; //무기 장착 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Weapon")
	bool bDrawEquipTraceDebug = false; //무기 장착 거리 디버그 선 그리기 여부

	//입력 함수들
	void MoveForward(float Value); //앞뒤 이동
	void MoveRight(float Value); //좌우 이동
	void Turn(float Value); //좌우 회전
	void LookUp(float Value); //상하 회전
	void StartJump(); //점프 시작
	void StopJump(); //점프 멈춤
	void ToggleCrouch(); //앉기/일어서기 토글
	void StartSprint(); //달리기 시작
	void StopSprint(); //달리기 멈춤
	void TryEquipWeapon(); //무기 장착 시도 함수
	void AttackPressed(); //공격 버튼 눌림
	void AttackReleased(); //공격 버튼 떼짐
	void AimButtonPressed(); //조준 버튼 눌림
	void AimButtonReleased(); //조준 버튼 떼짐
	void ReloadButtonPressed(); //재장전 버튼 눌림
	void AimOffset(float DeltaTime); //에임오프셋 계산
private:
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* ReloadMontage; // 재장전 몽타주

	UFUNCTION(Server, Reliable)
	void ServerStartSprint();
	UFUNCTION(Server, Reliable)
	void ServerStopSprint();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void UpdateMovementSpeed(); //움직임 속도 업데이트
	
	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting) // [Network] Sprint 상태 동기화 (OnRep 추가)
	bool bWantsToSprint = false; //달리기 의사 여부
	
	UFUNCTION()
	void OnRep_IsSprinting(); // [Network] Sprint 상태 변경 시 호출되는 함수

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Weapon", meta = (AllowPrivateAccess = "true"))
	AWeaponBase* OverlappingWeapon = nullptr; //장착된 무기 포인터

	FVector2D MoveInputVector = FVector2D::ZeroVector; //현재 이동 입력 상태 Sprint 가능 여부 판단용 (앞으로 갈때만 Sprint 가능)
	
	UPROPERTY(Replicated) // [Network] Turn In Place 상태 동기화
	bool bIsTurningInPlace = false; //제자리 회전 중인지 여부
	UPROPERTY(Replicated) // [Network] Aim Offset Yaw 동기화
	float AO_Yaw; //애니메이션 오프셋 Yaw 값
	float AO_Pitch; //애니메이션 오프셋 Pitch 값

	UPROPERTY(Replicated) // [Network] 절대 조준 Yaw 값 (For Stable IK)
	float NetAimYaw; 

public:
	FORCEINLINE float GetNetAimYaw() const { return NetAimYaw; }

	int32 MeleeAttackIndex = 0; // 근접 공격 콤보 인덱스
};
