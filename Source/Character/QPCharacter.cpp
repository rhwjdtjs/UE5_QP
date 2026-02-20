#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"
#include "PJ_Quiet_Protocol/Character/Controllers/QPPlayerController.h" 
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h" 
#include "PJ_Quiet_Protocol/Character/QPAniminstance.h"
#include "Camera/PlayerCameraManager.h"

AQPCharacter::AQPCharacter()
{
	PrimaryActorTick.bCanEverTick = true; //매 프레임마다 Tick 함수 호출 설정
	bUseControllerRotationYaw = false; //컨트롤러의 Yaw 회전에 따라 캐릭터 회전 안함
	bUseControllerRotationPitch = false; //컨트롤러의 Pitch 회전에 따라 캐릭터 회전 안함
	bUseControllerRotationRoll = false; //컨트롤러의 Roll 회전에 따라 캐릭터 회전 안함

	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	if (ensure(MoveComponent)) //무브먼트 컴포넌트가 유효한지 확인
	{
		MoveComponent->bOrientRotationToMovement = false; //이동 방향으로 캐릭터 회전 설정
		MoveComponent->bUseControllerDesiredRotation = false; //컨트롤러의 원하는 회전 설정
		MoveComponent->GetNavAgentPropertiesRef().bCanCrouch = true; //앉기 가능 설정
		MoveComponent->MaxWalkSpeed = WalkSpeed; //기본 걷기 속도 설정
		MoveComponent->MaxWalkSpeedCrouched = CrouchSpeed; //앉기 속도 설정
		MoveComponent->BrakingDecelerationWalking = 100.f; // 멈출 때 관성 추가 (기본값보다 낮게)
		MoveComponent->GroundFriction = 2.f; // 마찰력 감소로 부드러운 감속 유도
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

	SetNetUpdateFrequency(100.f); // [Network] 업데이트 빈도 상향 (66 -> 100)
	SetMinNetUpdateFrequency(66.f); // [Network] 최소 빈도 상향 (33 -> 66)

	// [Network] 먼 거리에서도 애니메이션 생략 없이 갱신 (부드러운 동작 보장)
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void AQPCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (CombatComponent)
	{
		CombatComponent->OnWeaponTypeChanged.AddDynamic(this, &AQPCharacter::HandleWeaponTypeChanged); //무기 타입 변경 이벤트 바인딩
		HandleWeaponTypeChanged(CombatComponent->GetEquippedWeaponType()); //현재 무기 타입 처리
		
		// [Fix] 조준 상태 변경 시 속도 업데이트 (Rubber Banding 방지)
		CombatComponent->OnAimStateChanged.AddDynamic(this, &AQPCharacter::HandleAimStateChanged);
	}
	if (CameraBoom)
	{
		DefaultArmLength = CameraArmLength; //기본 카메라 거리 저장
		CameraBoom->TargetArmLength = CameraArmLength; //카메라와 캐릭터 사이 거리 설정
		CameraBoom->SocketOffset = StandingCameraOffset; //서있을 때 카메라 오프셋 설정 (TargetOffset -> SocketOffset)
	}
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	if (MoveComponent)
	{
		MoveComponent->MaxWalkSpeedCrouched = CrouchSpeed; //앉기 속도 설정
		MoveComponent->GetNavAgentPropertiesRef().bCanCrouch = true; //앉기 가능 설정
	}

	UpdateMovementSpeed(); //움직임 속도 업데이트
	
	// 카메라 상하 회전 각도 제한
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (PlayerController->PlayerCameraManager)	
		{
			PlayerController->PlayerCameraManager->ViewPitchMin = -60.f; // 위를 보는 한계 (올려다봄)
			PlayerController->PlayerCameraManager->ViewPitchMax = 60.f;  // 아래를 보는 한계 (내려다봄) - 머리 위까지만
		}
	}
}

void AQPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (CameraBoom && CrouchCameraInterpSpeed > 0.f)
	{
		FVector DesiredOffset = GetDesiredCameraOffset(); // 현재 자세에 따른 원하는 카메라 오프셋 계산 (서있을 때 vs 앉아있을 때)
		float TargetArmLength = DefaultArmLength; // 기본 거리 (조준 여부와 관계없이 총을 들었을 때 적용)
		FVector TargetOffset = FVector::ZeroVector; // 카메라 피벗 위치 조정 (기본값은 캐릭터 위치)
		FRotator TargetRelRot = FRotator::ZeroRotator; // 카메라 상대 회전 (Pitch 조정용, 기본값은 회전 없음)

		// '총'을 들고 있는 경우 (조준 여부 무관하게 드론 뷰 적용)
		bool bIsHoldingGun = (Weapontype == EQPWeaponType::EWT_Rifle || Weapontype == EQPWeaponType::EWT_Shotgun || Weapontype == EQPWeaponType::EWT_Handgun);
		if (bIsHoldingGun) // 총을 들고 있는 경우 (조준 여부 무관하게 드론 뷰 적용)
		{
			float BaseArmLength = (IsAiming() ? AimingArmLength : DefaultArmLength) - 50.f; // 총을 들었을 때는 기본 거리에서 50cm 가까이 (Zoom In) - 조준 여부에 따라 기본 거리 조정 후 추가로 줌인
			TargetArmLength = BaseArmLength; // 총을 들었을 때는 기본 거리에서 50cm 가까이 (Zoom In) - 조준 여부에 따라 기본 거리 조정 후 추가로 줌인

			FRotator ControlRot = GetControlRotation();
			float Pitch = ControlRot.Pitch;
			if (Pitch > 180.f) // UE4의 Pitch는 0 ~ 360 범위이므로, -180 ~ 180 범위로 정규화
				Pitch -= 360.f;

			if (Pitch < 0.f)  // 아래를 볼 때는 Pivot 상승 + 카메라 숙임 + 줌인 적용
			{
				float AbsPitch = FMath::Abs(Pitch);

				// Pitch가 0에서 -90 사이일 때, Pivot을 최대 150cm까지 상승시키고, 카메라를 최대 35도까지 숙이며, 카메라 거리를 최대 100cm까지 줌인
				float AddedHeight = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, 150.f), AbsPitch);
				TargetOffset.Z = AddedHeight;

				// Pitch가 0에서 -90 사이일 때, 카메라를 최대 35도까지 숙이며, 카메라 거리를 최대 100cm까지 줌인
				float AddedPitch = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(0.f, -35.f), AbsPitch);
				TargetRelRot.Pitch = AddedPitch;
				TargetOffset.X = 0.f;

				// Pitch가 0에서 -90 사이일 때, 카메라 거리를 최대 100cm까지 줌인
				TargetArmLength = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(BaseArmLength, BaseArmLength - 100.f), AbsPitch);
			}
		}
		else // 총이 아닐 때는 기존 로직 유지 (Pitch에 따른 동적 줌)
		{
			if (IsAiming()) // 조준 중일 때는 Pitch에 따른 줌 대신 고정된 AimingArmLength 사용
			{
				TargetArmLength = AimingArmLength; 
			}
			else // 조준하지 않을 때는 Pitch에 따라 동적으로 줌인/줌아웃 적용
			{
				FRotator ControlRot = GetControlRotation();
				float Pitch = ControlRot.Pitch;
				if (Pitch > 180.f) // UE4의 Pitch는 0 ~ 360 범위이므로, -180 ~ 180 범위로 정규화
					Pitch -= 360.f;

				if (Pitch < 0.f) // 아래를 볼 때는 줌인 (카메라와 캐릭터 사이 거리 감소)
				{
					TargetArmLength = FMath::GetMappedRangeValueClamped(FVector2D(-90.f, 0.f), FVector2D(MinVerticalArmLength, DefaultArmLength), Pitch);
				}
				else // 위를 볼 때는 줌아웃 (카메라와 캐릭터 사이 거리 증가)
				{
					TargetArmLength = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 90.f), FVector2D(DefaultArmLength, MaxVerticalArmLength), Pitch);
				}
			}
		}
		
		// 카메라 위치와 회전을 부드럽게 보간하여 적용
		const FVector NewTargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, TargetOffset, DeltaTime, CrouchCameraInterpSpeed);
		CameraBoom->TargetOffset = NewTargetOffset;
		
		// SocketOffset는 카메라의 실제 위치에 영향을 주는 요소이므로, TargetOffset과 함께 보간하여 적용
		const FVector NewSocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, DesiredOffset, DeltaTime, CrouchCameraInterpSpeed);
		CameraBoom->SocketOffset = NewSocketOffset;

		// 카메라 회전 보간 (Pitch 조정용)
		if (FollowCamera)
		{
			FRotator NewRelRot = FMath::RInterpTo(FollowCamera->GetRelativeRotation(), TargetRelRot, DeltaTime, CrouchCameraInterpSpeed);
			FollowCamera->SetRelativeRotation(NewRelRot);
		}

		const float NewArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, CrouchCameraInterpSpeed); // 카메라 거리 보간
		CameraBoom->TargetArmLength = NewArmLength; // 카메라 거리 업데이트
	} 

	// 회전 모드 전환 로직 (Locally Controlled 또는 Authority인 경우에만 실행)
	if (IsLocallyControlled() || HasAuthority())
	{
		float Speed = GetVelocity().Size2D();
		UCharacterMovementComponent* MoveComponent = GetCharacterMovement();

		if (MoveComponent)
		{
			// 이동 중(Speed >= 5.0f): 무기를 들고 있으면 Strafe, 아니면 Normal
			if (Speed >= 5.0f)
			{
				bool bIsHoldingGun = (Weapontype == EQPWeaponType::EWT_Rifle || Weapontype == EQPWeaponType::EWT_Shotgun || Weapontype == EQPWeaponType::EWT_Handgun);

				if (IsAiming())
				{
					// 조준 시에는 즉각적인 반응을 위해 Controller Rotation 사용 (Crisp)
					bUseControllerRotationYaw = true;
					MoveComponent->bOrientRotationToMovement = false;
				}
				else if (bIsHoldingGun && !IsSprinting())
				{
					// 무기를 들고 있고 달리지 않을 때 (이동 방향과 상관없이 캐릭터가 바라보는 방향 유지)
					bUseControllerRotationYaw = false;
					MoveComponent->bOrientRotationToMovement = false;
				}
				else
				{
					//무기를 들고 있지 않거나 달릴 때
					bUseControllerRotationYaw = false;
					MoveComponent->bOrientRotationToMovement = true;
				}
			}
			// 정지 상태(Speed < 5.0f): Turn In Place를 위해 Controller Rotation 해제
			else
			{
				bUseControllerRotationYaw = false;
				MoveComponent->bOrientRotationToMovement = false;
			}
		}
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
	
	// Reload
	PlayerInputComponent->BindAction(TEXT("Reload"), IE_Pressed, this, &AQPCharacter::ReloadButtonPressed);

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
void AQPCharacter::StartSprint() //달리기 시작
{
	bWantsToSprint = true; 
	UpdateMovementSpeed();
	ServerStartSprint(); 
}
void AQPCharacter::StopSprint() //달리기 멈춤
{
	bWantsToSprint = false; 
	UpdateMovementSpeed(); 
	ServerStopSprint(); 
}

void AQPCharacter::ServerStartSprint_Implementation() //서버에서 달리기 시작 처리
{
	bWantsToSprint = true;
	UpdateMovementSpeed();
}

void AQPCharacter::ServerStopSprint_Implementation() //서버에서 달리기 멈춤 처리
{
	bWantsToSprint = false;
	UpdateMovementSpeed();
}

void AQPCharacter::OnRep_IsSprinting()
{
	UpdateMovementSpeed(); // SimProxy도 Sprint 상태 변경 시 속도 업데이트
}
void AQPCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) //앉기 시작 시 호출
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust); //부모 클래스의 OnStartCrouch 호출
	UpdateMovementSpeed(); 
}
void AQPCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) //일어서기 시작 시 호출
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust); //부모 클래스의 OnEndCrouch 호출
	UpdateMovementSpeed(); 
}
void AQPCharacter::UpdateMovementSpeed() 
{
	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	if (!MoveComponent) return; //무브먼트 컴포넌트가 없으면 함수 종료



	// [Moved to Tick] 회전 모드 업데이트 Logic 제거 (속도 기반 동적 제어를 위해)

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
void AQPCharacter::TryEquipWeapon() 
{
	if (!CombatComponent) return;
	if (OverlappingWeapon) //겹쳐진 무기가 있으면
	{
		// [Multiplayer] 서버에 장착 요청 (RPC)
		CombatComponent->ServerEquipWeapon(OverlappingWeapon);
		
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
	if (!CombatComponent) return; 

	CombatComponent->SetAiming(true); 
	UpdateMovementSpeed(); 
	
	if (CameraBoom) CameraBoom->bEnableCameraLag = false;
}

//조준 버튼에서 손을 뗐을 때 호출
void AQPCharacter::AimButtonReleased()
{
	if (!CombatComponent) return; 

	CombatComponent->SetAiming(false); 
	UpdateMovementSpeed(); 

	// 조준 해제 시 카메라 랙 다시 활성화
	if (CameraBoom) CameraBoom->bEnableCameraLag = true;
}

//현재 조준 중인지 여부를 외부에서 확인
bool AQPCharacter::IsAiming() const
{
	return CombatComponent && CombatComponent->IsAiming(); //전투 컴포넌트가 유효하고 조준 중인지 반환
}

// 네트워크 복제 설정
void AQPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQPCharacter, OverlappingWeapon);
	DOREPLIFETIME(AQPCharacter, bWantsToSprint);
	DOREPLIFETIME(AQPCharacter, bIsTurningInPlace);
	DOREPLIFETIME(AQPCharacter, bIsTurningInPlace);
	DOREPLIFETIME(AQPCharacter, NetAimYaw);
}

bool AQPCharacter::IsSprinting() const
{
	// 달리기 조건: 달리기 버튼이 눌려 있고, 조준 중이 아니며, 앞으로 이동 입력이 있는 경우
	if (!bWantsToSprint || IsAiming()) return false;

	if (!IsLocallyControlled()) // 서버와 다른 클라이언트는 실제 이동 속도로 판단
	{
		if (GetVelocity().SizeSquared2D() < 10.f) return false; // 10.f는 약간의 오차 범위 (1cm/s 이상)

		float Dot = FVector::DotProduct(GetVelocity().GetSafeNormal2D(), GetActorForwardVector()); // Dot이 0.1f 이상이면 대략 84도 이내로 전방 이동으로 간주 (약간의 횡이동 허용)
		return Dot > 0.1f; // 0.1f: 약간의 횡이동 허용
	}

	return MoveInputVector.X > 0.f; // 앞으로 이동 입력이 있는 경우에만 달리기로 간주 (후진은 달리기 아님)
}

void AQPCharacter::AimOffset(float DeltaTime)
{
	FRotator AimRotation = FRotator::ZeroRotator; // AimRotation 선언을 함수 시작 부분으로 이동

	// 1. 기본 회전값 획득 ( Pitch: Up(-), Down(+) )
	if (Controller)
	{
		AimRotation = Controller->GetControlRotation();
	}
	else
	{
		AimRotation = GetBaseAimRotation();
	}
	
	// [Network] NetAimYaw 업데이트 (Server/Local)
	if (HasAuthority() || IsLocallyControlled())
	{
		NetAimYaw = AimRotation.Yaw;
	}

	AimRotation.Pitch = FRotator::NormalizeAxis(AimRotation.Pitch); // Pitch 정규화 (0 ~ 360 -> -180 ~ 180)

	// Simulated Proxy도 AO 계산을 직접 수행 
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		float TargetPitch = FRotator::NormalizeAxis(AimRotation.Pitch); // SimProxy도 Pitch 정규화
		AO_Pitch = FMath::FInterpTo(AO_Pitch, TargetPitch, DeltaTime, 20.f);  
		AimRotation.Yaw = NetAimYaw;
	}
	else // Local/Server는 기존 로직 유지 (즉시 반영)
	{
		float Pitch = FRotator::NormalizeAxis(AimRotation.Pitch);
		AO_Pitch = FMath::Clamp(Pitch, -90.f, 90.f);
	}

	// 2. HitTarget 기반 보정 (Yaw Only)
	if (CombatComponent && !CombatComponent->HitTarget.IsZero())
	{
		FVector Start = GetActorLocation();

		// 근접 거리 체크 (2m 이상 거리에서만 LookAt 적용)
		if (FVector::Dist(Start, CombatComponent->HitTarget) > 200.f)
		{
			if (IsAiming()) // 조준 중일 때만 LookAt 적용 (비조준 시에는 기존 회전 유지)
			{
				FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, CombatComponent->HitTarget); // HitTarget을 향하는 회전 계산
				AimRotation.Yaw = LookAtRotation.Yaw; // Yaw만 적용하여 AimRotation 보정
			}
		}
	}

	// 3. Yaw 계산
	const float AimYaw = AimRotation.Yaw;
	const float ActorYaw = GetActorRotation().Yaw;

	const float DeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(
		FRotator(0.f, AimYaw, 0.f),
		FRotator(0.f, ActorYaw, 0.f)
	).Yaw;

	// Simulated Proxy는 부드러운 보간 적용, Local/Server는 즉시 반영 (회전 애니메이션이 어색하게 보이는 것을 방지)
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		float TargetYaw = FMath::Clamp(DeltaYaw, -90.f, 90.f);
		AO_Yaw = FMath::FInterpTo(AO_Yaw, TargetYaw, DeltaTime, 5.f); // 10.f -> 5.f (Micro-Jitter Fix)
	}
	else
	{
		AO_Yaw = FMath::Clamp(DeltaYaw, -90.f, 90.f);
	}

	// 이동 중인지 확인
	const float Speed = GetVelocity().Size2D();
	bool bIsAttacking = false;
	if (CombatComponent) bIsAttacking = CombatComponent->IsAttacking();

	const float DeltaYawAbs = FMath::Abs(DeltaYaw); 

	{
		if (Speed > 1.f) // 이동 중이라면 (1.f는 오차 범위)
		{
			bIsTurningInPlace = false;

			const FRotator TargetRotation = FRotator(0.f, AimYaw, 0.f); // 이동 중에는 항상 AimYaw를 향하도록 회전 (달리기 중에도 적용)
			

			if (GetLocalRole() != ROLE_SimulatedProxy)  // Simulated Proxy는 제자리 회전 로직에서 제외 (회전 애니메이션이 어색하게 보이는 것을 방지)
			{
				if (!bUseControllerRotationYaw) // Controller Rotation이 비활성화된 경우에만 회전 로직 적용 (달리기 중에는 Controller Rotation이 활성화되어 있으므로 회전 로직 적용 제외)
				{
					if (IsAiming()) // 조준 중일 때만 즉시 회전 (공격 중 강제 회전 제거)
					{
						SetActorRotation(TargetRotation);
					}
					else // 이동 중이지만 조준하지 않을 때는 부드럽게 회전
					{
						// 몽타주 재생 여부 확인
						bool bIsMontagePlaying = false;
						if (GetMesh() && GetMesh()->GetAnimInstance())
						{
							if (CombatComponent && CombatComponent->GetEquippedWeapon())
							{
								UAnimMontage* FireMontage = CombatComponent->GetEquippedWeapon()->GetFireMontage();
								if (GetMesh()->GetAnimInstance()->Montage_IsPlaying(FireMontage))
								{
									bIsMontagePlaying = true;
								}
							}
						}

						float InterpSpeed = 15.f;
						if (bIsAttacking || bIsMontagePlaying) InterpSpeed = 50.f; // [Fix] 공격 중에는 빠르게 회전하여 상체 비틀림 방지

						const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, InterpSpeed); // 이동 중이지만 조준하지 않을 때는 부드럽게 회전 (공격 중 강제 회전 제거)
						SetActorRotation(NewRotation);

						// 회전 후의 Yaw로 AO_Yaw 계산 
						const float NewActorYaw = NewRotation.Yaw;
						const float NewDeltaYaw = UKismetMathLibrary::NormalizedDeltaRotator(FRotator(0.f, AimYaw, 0.f), FRotator(0.f, NewActorYaw, 0.f)).Yaw;
						AO_Yaw = FMath::Clamp(NewDeltaYaw, -90.f, 90.f);
					}
				}
			}
		}
		else // 거의 정지 상태라면 제자리 회전 로직 적용
		{
			
			float TurnThreshold = 60.f; 
			if (bIsAttacking) TurnThreshold = 0.f; // [Fix] 공격 중에는 즉시 회전하여 상체 비틀림 방지

			if (DeltaYawAbs > TurnThreshold) // 60도 이상으로 벌어지면 제자리 회전 시작
			{
				bIsTurningInPlace = true;
			}
			else if (DeltaYawAbs < 5.f) // 5도 미만으로 줄어들면 제자리 회전 종료 (부드러운 종료를 위해 완화)
			{
				bIsTurningInPlace = false;
			}

			if (bIsTurningInPlace) // 제자리 회전 로직 적용
			{
				const FRotator TargetRotation = FRotator(0.f, AimYaw, 0.f); 

				float InterpSpeed = 20.f; // 기본 회전 속도
				if (DeltaYawAbs <= 60.f) // 60도 이하에서는 회전 속도를 점점 빠르게 (잔여 회전이 적을수록 더 빠르게)
				{
					InterpSpeed = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 60.f), FVector2D(10.f, 20.f), DeltaYawAbs); // 0도에 가까울수록 20.f에 가깝게, 60도에 가까울수록 10.f에 가깝게 (잔여 회전이 적을수록 더 빠르게)
				}

				if (GetLocalRole() != ROLE_SimulatedProxy) // Simulated Proxy는 제자리 회전 로직에서 제외 (회전 애니메이션이 어색하게 보이는 것을 방지)
				{
					if (DeltaYawAbs < 2.f) // 2도 미만으로 줄어들면 회전을 강제로 맞춰서 제자리 회전 종료 (잔여 회전이 거의 없을 때는 부드러운 종료를 위해 강제 맞춤)
					{
						SetActorRotation(TargetRotation);
						bIsTurningInPlace = false; // 강제 종료
					}
					else // 잔여 회전이 아직 있을 때는 부드럽게 회전
					{
						const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, InterpSpeed);
						SetActorRotation(NewRotation);
					}
				}
			}
		}
	}

	// 디버그용
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Green, FString::Printf(TEXT("Turning: %s | AO_Yaw: %.2f | Delta: %.2f"),
			bIsTurningInPlace ? TEXT("ON") : TEXT("OFF"), AO_Yaw, DeltaYawAbs));

		FString RoleStr = GetLocalRole() == ROLE_Authority ? TEXT("Server") : TEXT("Client");
		FString HitTargetStr = CombatComponent ? CombatComponent->HitTarget.ToString() : TEXT("None");
		
		float Pitch = AimRotation.Pitch; 

		GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Red, 
			FString::Printf(TEXT("[%s] Pitch: %.2f | AO_P: %.2f | AO_Y: %.2f | HitTarget: %s"), 
			*RoleStr, Pitch, AO_Pitch, AO_Yaw, *HitTargetStr));
	}
}

// 무기 발사 몽타주 재생 함수
void AQPCharacter::PlayFireMontage(bool bAming)
{
	if (!CombatComponent) return; //전투 컴포넌트가 없으면 함수 종료
	
	AWeaponBase* EquippedWeapon = CombatComponent->GetEquippedWeapon(); //장착된 무기 가져오기
	if (!EquippedWeapon) return; //장착된 무기가 없으면 함수 종료

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); //애니메이션 인스턴스 가져오기
	UAnimMontage* MontageToPlay = EquippedWeapon->GetFireMontage();
	
	FName SectionName; //재생할 몽타주 섹션 이름 결정

	switch (Weapontype) 
	{
	case EQPWeaponType::EWT_Rifle:
		SectionName = bAming ? FName("RifleAim") : FName("RifleHip");
		break;
	case EQPWeaponType::EWT_Shotgun:
		SectionName = bAming ? FName("ShotgunAim") : FName("ShotgunHip");
		break;
	case EQPWeaponType::EWT_Handgun:
		SectionName = bAming ? FName("HandgunAim") : FName("HandgunHip");
		break;
	case EQPWeaponType::EWT_Melee:
		if (MeleeAttackIndex == 0) // 첫 번째 공격이면 Attack1 섹션 재생
		{
			SectionName = FName("Attack1");
			MeleeAttackIndex = 1;
		}
		else // 두 번째 공격이면 Attack2 섹션 재생
		{
			SectionName = FName("Attack2");
			MeleeAttackIndex = 0;
		}
		break;
	default:
		SectionName = NAME_None; // 기본 섹션 이름 (무기 타입이 정의되지 않은 경우)
		break;
	}

	if (AnimInstance && MontageToPlay) //애니메이션 인스턴스와 발사 몽타주가 유효하면
	{
		AnimInstance->Montage_Play(MontageToPlay); //발사 몽타주 재생
		if (SectionName != NAME_None) //섹션 이름이 유효하면 해당 섹션으로 점프
		{
			UE_LOG(LogTemp, Warning, TEXT("Playing Montage: %s | Section: %s"), *MontageToPlay->GetName(), *SectionName.ToString()); 
			
			AnimInstance->Montage_JumpToSection(SectionName, MontageToPlay); //지정된 섹션으로 점프
		}
		else //섹션 이름이 유효하지 않으면 기본 섹션으로 재생
		{
			UE_LOG(LogTemp, Warning, TEXT("Playing Montage: %s | Default Section (No Name)"), *MontageToPlay->GetName());
		}
	}
}

void AQPCharacter::ReloadButtonPressed() //재장전 버튼이 눌렸을 때 호출
{
	if (CombatComponent)
	{
		CombatComponent->Reload();
	}
}

void AQPCharacter::PlayReloadMontage() //재장전 몽타주 재생 함수
{
	if (!CombatComponent || !ReloadMontage) return; //전투 컴포넌트나 재장전 몽타주가 없으면 함수 종료

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage) 
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Weapontype) 
		{
		case EQPWeaponType::EWT_Rifle:
			SectionName = FName("Rifle");
			break;
		case EQPWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EQPWeaponType::EWT_Handgun:
			SectionName = FName("Pistol");
			break;
		}

		if (!SectionName.IsNone()) //섹션 이름이 유효하면 해당 섹션으로 점프
		{
			AnimInstance->Montage_JumpToSection(SectionName, ReloadMontage); 
		}
	}
}

void AQPCharacter::HandleAimStateChanged(bool bIsAiming)
{
	UpdateMovementSpeed(); //조준 상태가 변경되면 즉시 이동 속도 재계산 (애니메이션 연결 문제 테스트 중)
}



