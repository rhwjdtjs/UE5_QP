#include "QPCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
#include "Controllers/QPPlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "PJ_Quiet_Protocol/Character/QPAniminstance.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"

AQPCharacter::AQPCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = true; //컨트롤러의 Yaw 회전에 따라 캐릭터 회전 안함
	bUseControllerRotationPitch = false; //컨트롤러의 Pitch 회전에 따라 캐릭터 회전 안함
	bUseControllerRotationRoll = false; //컨트롤러의 Roll 회전에 따라 캐릭터 회전 안함

	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	if (ensure(MoveComponent)) //무브먼트 컴포넌트가 유효한지 확인
	{
		MoveComponent->bOrientRotationToMovement = false; //이동 방향으로 캐릭터 회전 설정
		MoveComponent->bUseControllerDesiredRotation = true; //
		MoveComponent->GetNavAgentPropertiesRef().bCanCrouch = true; //앉기 가능 설정
		MoveComponent->MaxWalkSpeed = WalkSpeed; //기본 걷기 속도 설정
		MoveComponent->MaxWalkSpeedCrouched = CrouchSpeed; //앉기 속도 설정
	}
	//카메라 붐 설정
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); //스프링암 컴포넌트 생성
	CameraBoom->SetupAttachment(RootComponent); //루트 컴포넌트에 부착
	CameraBoom->TargetArmLength = CameraArmLength; //카메라와 캐릭터 사이 거리 설정
	CameraBoom->bUsePawnControlRotation = true; //컨트롤러 회전에 따라 카메라 회전 설정
	CameraBoom->bEnableCameraLag = true; //카메라 지연 활성화
	CameraBoom->CameraLagSpeed = 10.f; //카메라 지연 속도 설정

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera")); //카메라 컴포넌트 생성
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); //카메라 붐에 부착
	FollowCamera->bUsePawnControlRotation = false; //카메라가 폰의 회전에 따라 회전하지 않도록 설정

	//컴뱃 컴포넌트
	CombatComponent = CreateDefaultSubobject<UQPCombatComponent>(TEXT("CombatComponent")); //전투 컴포넌트 생성
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent")); //인벤토리 컴포넌트 생성
}

void AQPCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (CombatComponent)
	{
		CombatComponent->OnWeaponTypeChanged.AddDynamic(this, &AQPCharacter::HandleWeaponTypeChanged); //무기 타입 변경 이벤트 바인딩
		HandleWeaponTypeChanged(CombatComponent->GetEquippedWeaponType()); //현재 무기 타입 처리
	}
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = CameraArmLength; //카메라와 캐릭터 사이 거리 설정
		CameraBoom->TargetOffset = StandingCameraOffset; //서있을 때 카메라 오프셋 설정
	}
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	if (MoveComponent)
	{
		MoveComponent->MaxWalkSpeedCrouched = CrouchSpeed; //앉기 속도 설정
		MoveComponent->GetNavAgentPropertiesRef().bCanCrouch = true; //앉기 가능 설정
	}

	UpdateMovementSpeed(); //움직임 속도 업데이트
}

void AQPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (CameraBoom && CrouchCameraInterpSpeed > 0.f)
	{
		const FVector DesiredOffset = GetDesiredCameraOffset(); //원하는 카메라 오프셋 계산
		const FVector NewOffset = FMath::VInterpTo(CameraBoom->TargetOffset, DesiredOffset, DeltaTime, CrouchCameraInterpSpeed); //카메라 오프셋 보간
		CameraBoom->TargetOffset = NewOffset; //카메라 붐의 타겟 오프셋 업데이트
	}
	AimOffset(DeltaTime); //회전 차이 계산
}

void AQPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent); //입력 컴포넌트 유효성 검사
	PlayerInputComponent->BindAxis("MoveForward", this, &AQPCharacter::MoveForward); //앞뒤 이동 바인딩
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AQPCharacter::MoveRight); //좌우 이동 바인딩
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AQPCharacter::Turn); //좌우 회전 바인딩
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AQPCharacter::LookUp); //상하 회전 바인딩
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AQPCharacter::StartJump); //점프 바인딩
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AQPCharacter::StopJump); //점프 멈춤 바인딩
	PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &AQPCharacter::ToggleCrouch); //앉기/일어서기 토글 바인딩
	// Sprint (Hold)
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AQPCharacter::StartSprint); //달리기 시작 바인딩
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AQPCharacter::StopSprint); //달리기 멈춤 바인딩
	// input 추가
	PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AQPCharacter::AttackPressed); //발사 바인딩
	PlayerInputComponent->BindAction(TEXT("Attack"), IE_Released, this, &AQPCharacter::AttackReleased); //발사 멈춤 바인딩
	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Pressed, this, &AQPCharacter::TryEquipWeapon); //장착 바인딩
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Pressed, this, &AQPCharacter::AimButtonPressed); //조준 바인딩
	PlayerInputComponent->BindAction(TEXT("Aim"), IE_Released, this, &AQPCharacter::AimButtonReleased); //조준 멈춤 바인딩
}
void AQPCharacter::SetOverlappingWeapon(AWeaponBase* Weapon)
{
	if (OverlappingWeapon == Weapon) return; //겹쳐진 무기가 이미 설정된 무기와 같으면 함수 종료
	OverlappingWeapon = Weapon; //겹쳐진 무기 설정
	if (!IsLocallyControlled()) return; //로컬에서 제어되지 않는 경우 함수 종료
	if (AQPPlayerController* PlayerController = Cast<AQPPlayerController>(GetController())) //플레이어 컨트롤러 가져오기
	{
		PlayerController->SetPickupTarget(OverlappingWeapon); //픽업 타겟 설정
	}
}
void AQPCharacter::HandleWeaponTypeChanged(EQPWeaponType NewWeaponType)
{
	Weapontype = NewWeaponType; //장착된 무기 타입 업데이트
}
//움직임 함수들
void AQPCharacter::MoveForward(float Value) //앞뒤 이동
{
	if (!Controller) return; //컨트롤러가 없거나 

	MoveInputVector.X = Value; // 항상 전 후 입력값 갱신
	UpdateMovementSpeed(); //방향이 바뀌면 속도 재계산

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f); //컨트롤러의 Yaw 회전 가져오기
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X); //앞 방향 벡터 계산
	AddMovementInput(Direction, Value); //이동 입력 추가
}
void AQPCharacter::MoveRight(float Value)
{
	if (!Controller) return; //컨트롤러가 없거나

	MoveInputVector.Y = Value; // 항상 좌 우 입력값 갱신 
	UpdateMovementSpeed(); // 방향이 바뀌면 속도 재계산

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f); //컨트롤러의 Yaw 회전 가져오기
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y); //오른쪽 방향 벡터 계산
	AddMovementInput(Direction, Value); //이동 입력 추가
}
void AQPCharacter::Turn(float Value) { AddControllerYawInput(Value); }  //컨트롤러의 Yaw 입력 추가
void AQPCharacter::LookUp(float Value) { AddControllerPitchInput(Value); } //컨트롤러의 Pitch 입력 추가
void AQPCharacter::StartJump() { Jump(); } //점프 시작
void AQPCharacter::StopJump() { StopJumping(); } //점프 멈춤
void AQPCharacter::ToggleCrouch()
{
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	if (!MoveComponent || !MoveComponent->GetNavAgentPropertiesRef().bCanCrouch) return; //무브먼트 컴포넌트가 없거나 앉기 불가능하면 함수 종료
	if (bIsCrouched)
	{
		UnCrouch(); //일어서기
	}
	else
	{
		Crouch(); //앉기
	}
}
void AQPCharacter::StartSprint()
{
	bWantsToSprint = true; //달리기 의사 설정
	UpdateMovementSpeed(); //움직임 속도 업데이트
}
void AQPCharacter::StopSprint()
{
	bWantsToSprint = false; //달리기 의사 해제
	UpdateMovementSpeed(); //움직임 속도 업데이트
}
void AQPCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust); //부모 클래스의 OnStartCrouch 호출
	UpdateMovementSpeed(); //움직임 속도 업데이트
}
void AQPCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust); //부모 클래스의 OnEndCrouch 호출
	UpdateMovementSpeed(); //움직임 속도 업데이트
}
void AQPCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	if (!MoveComponent) return; //무브먼트 컴포넌트가 없으면 함수 종료

	/**
	 * 속도 우선순위
	 * 1. 앉아 있음 + W + Shift → CrouchSprintSpeed
	 * 2. 서 있음 + W + Shift → SprintSpeed
	 * 3. 앉아 있음 → CrouchSpeed
	 * 4. 그 외 → WalkSpeed
	 */
	if (IsSprinting()) //달리기 중인지 확인
	{
		if (bIsCrouched) //앉아 있는지 확인
		{
			MoveComponent->MaxWalkSpeedCrouched = CrouchSprintSpeed; //앉아서 뛰는 속도
		}
		else
		{
			MoveComponent->MaxWalkSpeed = SprintSpeed; //일어서서 뛰는 속도
		}
	}
	else if (bIsCrouched) //앉아 있는지 확인
	{
		MoveComponent->MaxWalkSpeedCrouched = CrouchSpeed; //앉은 상태일 때 걷는 속도
	}
	else
	{
		MoveComponent->MaxWalkSpeed = WalkSpeed; //서있을 때의 걷는 속도
	}

}

//전투 (Combat) 관련 함수들
void AQPCharacter::TryEquipWeapon() {
	if (!CombatComponent) return;
	if (OverlappingWeapon) //겹쳐진 무기가 있으면
	{
		CombatComponent->EquipWeapon(OverlappingWeapon, true); //무기 장착 시도
		OverlappingWeapon = nullptr; //겹쳐진 무기 초기화
		SetOverlappingWeapon(nullptr); //겹쳐진 무기 설정 함수 호출
	}
}
void AQPCharacter::AttackPressed()
{
	if (CombatComponent) CombatComponent->StartAttack(); //공격 시작
}
void AQPCharacter::AttackReleased()
{
	if (CombatComponent) CombatComponent->StopAttack(); //공격 멈춤
}

//조준 버튼을 눌렀을 때 호출
void AQPCharacter::AimButtonPressed()
{
	if (CombatComponent) //전투 컴포넌트가 유효하면
	{
		CombatComponent->SetAiming(true); //조준 시작
	}

}

//조준 버튼에서 손을 뗐을 때 호출
void AQPCharacter::AimButtonReleased()
{
	if (CombatComponent) //전투 컴포넌트가 유효하면
	{
		CombatComponent->SetAiming(false); //조준 멈춤
	}

}

//현재 조준 중인지 여부를 외부에서 확인
bool AQPCharacter::IsAiming()
{
	return CombatComponent && CombatComponent->IsAiming(); //전투 컴포넌트가 유효하고 조준 중인지 반환
}

// 회전 차이 계산
void AQPCharacter::AimOffset(float DeltaTime)
{
	if (!Controller) return; //컨트롤러가 없으면 함수 종료

	const FVector Velocity = GetVelocity(); //캐릭터의 현재 속도 벡터 가져오기
	const float Speed = FVector(Velocity.X, Velocity.Y, 0.f).Size(); //수평 속도 크기 계산
	const bool bIsInAir = GetCharacterMovement()->IsFalling(); //캐릭터가 공중에 있는지 확인

	const FRotator CurrentAimRotation(0.f, GetBaseAimRotation().Yaw, 0.f); //현재 에임 회전 계산

	if (Speed == 0.f && !bIsInAir) //멈춰있고 공중에 떠있지 않을 때
	{
		// 현재 AimRotation과 이전 AimRotation 차이
		const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation); //회전 차이 계산

		AO_Yaw = Delta.Yaw; // Yaw 차이 설정
		const float YawLimit = 91.f; // Yaw 제한 값 (최대 값 설정하기)
		const float ReduceAmount = 3.f; // Yaw 감소량 (Yaw 값을 서서히 줄이기 위한 값)

		if (FMath::Abs(AO_Yaw) > YawLimit) // Yaw 값이 제한 값을 초과할 때
		{
			AO_Yaw -= FMath::Sign(AO_Yaw) * ReduceAmount; // Yaw 값을 감소시켜 제한 값 내로 유지
			StartingAimRotation.Yaw += FMath::Sign(AO_Yaw) * ReduceAmount; // 기준 회전도 함께 조정
		}
	}
	else // 이동 중일 때
	{
		// 이동 중에는 기준을 계속 갱신
		StartingAimRotation = CurrentAimRotation; // 기준 회전 갱신
		AO_Yaw = 0.f; // Yaw 차이 초기화
	}

	AO_Pitch = GetBaseAimRotation().Pitch; // Pitch 값 설정
}

// 무기 발사 몽타주 재생 함수
void AQPCharacter::PlayFireMontage(bool bAming)
{
	if (!CombatComponent) return; //전투 컴포넌트가 없으면 함수 종료

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); //애니메이션 인스턴스 가져오기

	if (AnimInstance && FireWeaponMontage) //애니메이션 인스턴스와 발사 몽타주가 유효하면
	{
		AnimInstance->Montage_Play(FireWeaponMontage); //발사 몽타주 재생
		FName SectionName; //재생할 섹션 이름 변수
		SectionName = bAming ? FName("RifleAim") : FName("RifleHip"); //조준 중이면 "RifleAim", 아니면 "RifleHip"
		AnimInstance->Montage_JumpToSection(SectionName, FireWeaponMontage); //지정된 섹션으로 점프

	}
}