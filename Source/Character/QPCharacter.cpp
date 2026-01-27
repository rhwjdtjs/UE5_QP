#include "QPCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "PJ_Quiet_Protocol/Character/Components/QPCombatComponent.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
#include "Controllers/QPPlayerController.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryComponent.h"
#include "PJ_Quiet_Protocol/Inventory/WorldItemActor.h"
#include "PJ_Quiet_Protocol/Inventory/InventoryHeaders/InventoryItem.h"


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

void AQPCharacter::SetOverlappingWorldItem(AWorldItemActor* WorldItem)
{
	if (OverlappingWorldItem == WorldItem) return; //겹쳐진 월드 아이템이 이미 설정된 아이템과 같으면 함수 종료
	OverlappingWorldItem = WorldItem; //겹쳐진 월드 아이템 설정
	UpdatePickupWidgetTarget(); //픽업 위젯 타겟 업데이트
}

void AQPCharacter::EquipInventoryItemAt(const FIntPoint& Cell)
{
	if (!InventoryComponent || !CombatComponent) return; //인벤토리 컴포넌트나 전투 컴포넌트가 없으면 함수 종료

	FInventorySlot Slot; //인벤토리 슬롯 변수 선언
	if (!InventoryComponent->FindSlotContaining(Cell, Slot)) return; //해당 셀에 아이템이 없으면 함수 종료
	if (!Slot.Item.ItemData) return; //아이템 데이터가 없으면 함수 종료

	// 무기 아이템만 장착 처리
	if (Slot.Item.ItemData->ItemType != EItemType::EIT_Weapon) return; //아이템 타입이 무기가 아니면 함수 종료
	if (!Slot.Item.ItemData->WeaponClass) return; //무기 클래스가 없으면 함수 종료

	//기존 장착 무기 드랍 없이 해제
	if (CombatComponent->HasWeapon()) //기존에 장착된 무기가 있으면
	{
		CombatComponent->UnEquipWeapon(false); //장착 해제
	}

	FActorSpawnParameters Params; //액터 스폰 파라미터 설정
	Params.Owner = this; //소유자 설정
	Params.Instigator = this; //인스티게이터 설정
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; //충돌 처리 방법 설정

	AWeaponBase* NewWeapon = GetWorld()->SpawnActor<AWeaponBase>(Slot.Item.ItemData->WeaponClass, Params); //무기 액터 스폰
	if (!NewWeapon) return; //무기 스폰 실패 시 함수 종료

	// 장착 성공 시 인벤에서 제거
	if (CombatComponent->EquipWeapon(NewWeapon, false)) //무기 장착 시도
	{
		InventoryComponent->RemoveItemAt(Slot.Position); //인벤토리에서 아이템 제거
	}
	else //장착 실패 시 스폰된 무기 파괴
	{
		NewWeapon->Destroy(); //무기 액터 파괴
	} //장착 성공 시 인벤에서 제거
}

void AQPCharacter::DropInventoryItemAt(const FIntPoint& Cell)
{
	if (!InventoryComponent) return; // 인벤 컴포넌트가 없으면 종료

	FInventorySlot Slot; // 슬롯 데이터
	if (!InventoryComponent->FindSlotContaining(Cell, Slot)) return; // 해당 셀에 아이템이 없으면 종료
	if (!Slot.Item.ItemData) return; // 아이템 데이터가 없으면 종료

	FVector DropLoc = GetActorLocation() + GetActorForwardVector() * 150.f; // 기본 드랍 위치는 캐릭터 앞

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) // 컨트롤러가 PC면
	{
		FHitResult Hit; // 커서 히트 결과
		if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, true, Hit)) // 커서 아래 바닥을 찍으면
		{
			DropLoc = Hit.Location + FVector(0.f, 0.f, 20.f); // 바닥 위로 약간 띄워서 드랍
		}
	}

	UItemDataAsset* ItemData = Slot.Item.ItemData; // 아이템 데이터
	const int32 Quantity = Slot.Item.Quantity; // 수량

	if (ItemData->ItemType == EItemType::EIT_Weapon && ItemData->WeaponClass) // 무기 타입이고 무기 클래스가 있으면
	{
		FActorSpawnParameters Params; // 스폰 파라미터
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn; // 가능한 위치로 스폰
		Params.Owner = nullptr; // 월드 드랍이므로 소유자 없음
		Params.Instigator = nullptr; // 인스티게이터 없음

		AWeaponBase* DroppedWeapon = GetWorld()->SpawnActor<AWeaponBase>(ItemData->WeaponClass, DropLoc, FRotator::ZeroRotator, Params); // 무기 액터 스폰
		if (DroppedWeapon)
		{
			// 무기는 BP_GunWeapon 같은 액터 자체가 월드 픽업 대상
			// 무기 드랍 로직은 WeaponBase에서 처리
		}

		InventoryComponent->RemoveItemAt(Slot.Position); // 인벤에서 제거
		return; // 무기 드랍 처리 끝
	}

	// 무기가 아닌 일반 아이템은 WorldItemActor로 드랍
	FActorSpawnParameters Params; // 스폰 파라미터
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn; // 가능한 위치로 스폰

	AWorldItemActor* Dropped = GetWorld()->SpawnActor<AWorldItemActor>(AWorldItemActor::StaticClass(), DropLoc, FRotator::ZeroRotator, Params); // 월드 아이템 액터 스폰
	if (Dropped)
	{
		Dropped->ItemData = ItemData; // 아이템 데이터 설정
		Dropped->Quantity = Quantity; // 수량 설정
	}

	InventoryComponent->RemoveItemAt(Slot.Position); // 인벤에서 제거
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

	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Pressed, this, &AQPCharacter::EquipPressed); //장착 바인딩
	PlayerInputComponent->BindAction(TEXT("Equip"), IE_Released, this, &AQPCharacter::EquipReleased); //장착 해제 바인딩

	PlayerInputComponent->BindAction(TEXT("Drop"), IE_Pressed, this, &AQPCharacter::DropPressed); //드랍 바인딩
	PlayerInputComponent->BindAction(TEXT("Drop"), IE_Released, this, &AQPCharacter::DropReleased); //드랍 해제 바인딩
}
void AQPCharacter::EquipPressed()
{
	bEquipKeyDown = true; //장착 키 눌림 상태 설정
	bEquipHoldConsumed = false; //장착 홀드 소비 상태 초기화

	GetWorldTimerManager().ClearTimer(EquipHoldTimerHandle); //타이머 초기화
	GetWorldTimerManager().SetTimer(EquipHoldTimerHandle, this, &AQPCharacter::OnEquipHoldTriggered, EquipHoldThreshhold, false); //장착 홀드 타이머 설정
}
void AQPCharacter::EquipReleased()
{
	bEquipKeyDown = false; //장착 키 눌림 상태 해제
	GetWorldTimerManager().ClearTimer(EquipHoldTimerHandle); //타이머 초기화
	if (bEquipHoldConsumed) //장착 홀드가 소비되지 않은 경우
	{
		bEquipHoldConsumed = false; //장착 홀드 소비 상태 초기화
		return; //함수 종료
	}
	TryEquipWeapon(); //무기 장착 시도 짧게 눌렀을때
}
void AQPCharacter::TryEquipWeapon() {
	if (!CombatComponent) return;
	if (OverlappingWeapon) //겹쳐진 무기가 있으면
	{
		CombatComponent->EquipWeapon(OverlappingWeapon, true); //무기 장착 시도
		OverlappingWeapon = nullptr; //겹쳐진 무기 초기화
		SetOverlappingWeapon(nullptr); //겹쳐진 무기 설정 함수 호출
		return;
	}
	if(OverlappingWorldItem && InventoryComponent) //겹쳐진 월드 아이템이 있고 인벤토리 컴포넌트가 있으면
	{
		UItemDataAsset* ItemData = OverlappingWorldItem->ItemData; //겹쳐진 월드 아이템의 아이템 데이터 가져오기
		const int32 Quantity = OverlappingWorldItem->Quantity; //겹쳐진 월드 아이템의 수량 가져오기
		if (ItemData && Quantity > 0)
		{
			const bool bAdded = InventoryComponent->AddItem(ItemData, Quantity); //아이템 인벤토리에 추가 시도
			if (bAdded)
			{
				AWorldItemActor* Picked = OverlappingWorldItem; //픽업한 월드 아이템 저장
				SetOverlappingWorldItem(nullptr); //겹쳐진 월드 아이템 초기화
				Picked->Destroy(); //월드 아이템 액터 파괴
			}
		}
	}
}
//E 홀드(길게): 무기면 인벤 저장(장착 안함), 아이템이면 인벤 추가
void AQPCharacter::TryStorePickupToInventory()
{
	if (!InventoryComponent) return; //인벤토리 컴포넌트가 없으면 함수 종료

	if (OverlappingWeapon)
	{
		UItemDataAsset* WeaponItemData = OverlappingWeapon->GetWeaponItemData(); //겹쳐진 무기의 아이템 데이터 가져오기
		if (!WeaponItemData) return; //아이템 데이터가 없으면 함수 종료

		const bool bAdded = InventoryComponent->AddItem(WeaponItemData, 1); //아이템 인벤토리에 추가 시도
		if (bAdded)
		{
			AWeaponBase* PickedWeapon = OverlappingWeapon; //픽업한 무기 저장
			SetOverlappingWeapon(nullptr); //겹쳐진 무기 초기화
			PickedWeapon->Destroy(); //무기 액터 파괴
		}
		return;
	}
	// 월드 아이템: 인벤토리 추가
	if (OverlappingWorldItem)
	{
		UItemDataAsset* ItemData = OverlappingWorldItem->ItemData; //겹쳐진 월드 아이템의 아이템 데이터 가져오기
		const int32 Quantity = OverlappingWorldItem->Quantity; //겹쳐진 월드 아이템의 수량 가져오기
		if (ItemData && Quantity > 0)
		{
			const bool bAdded = InventoryComponent->AddItem(ItemData, Quantity); //아이템 인벤토리에 추가 시도
			if (bAdded)
			{
				AWorldItemActor* Picked = OverlappingWorldItem; //픽업한 월드 아이템 저장
				SetOverlappingWorldItem(nullptr); //겹쳐진 월드 아이템 초기화
				Picked->Destroy(); //월드 아이템 액터 파괴
			}
		}
	}
}
void AQPCharacter::OnEquipHoldTriggered()
{
	if (!bEquipKeyDown) return; //장착 키가 눌리지 않은 경우 함수 종료
	bEquipHoldConsumed = true; //장착 홀드 소비 상태 설정
	TryStorePickupToInventory(); //인벤토리에 픽업 저장 시도
}
void AQPCharacter::SetOverlappingWeapon(AWeaponBase* Weapon)
{
	if (OverlappingWeapon == Weapon) return; //겹쳐진 무기가 이미 설정된 무기와 같으면 함수 종료
	OverlappingWeapon = Weapon; //겹쳐진 무기 설정
	UpdatePickupWidgetTarget(); //픽업 위젯 타겟 업데이트
	
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
	if (IsSprinting())
	{
		if (bIsCrouched)
		{
			MoveComponent->MaxWalkSpeedCrouched = CrouchSprintSpeed; //앉아서 뛰는 속도
		}
		else
		{
			MoveComponent->MaxWalkSpeed = SprintSpeed; //일어서서 뛰는 속도
		}
	}
	else if (bIsCrouched)
	{
		MoveComponent->MaxWalkSpeedCrouched = CrouchSpeed; //앉은 상태일 때 걷는 속도
	}
	else
	{
		MoveComponent->MaxWalkSpeed = WalkSpeed; //서있을 때의 걷는 속도
	}

}
void AQPCharacter::UpdatePickupWidgetTarget()
{
	if (!IsLocallyControlled()) return; //로컬 컨트롤러가 아니면 함수 종료

	AActor* NewTarget = nullptr; //새로운 타겟 액터 초기화
	if(OverlappingWeapon) NewTarget = OverlappingWeapon; //겹쳐진 무기가 있으면 타겟 설정
	else if (OverlappingWorldItem) NewTarget = OverlappingWorldItem; //겹쳐진 월드 아이템이 있으면 타겟 설정

	if(AQPPlayerController* PlayerController = Cast<AQPPlayerController>(GetController()))
	{
		PlayerController->SetPickupTarget(NewTarget); //픽업 타겟 설정
	}
}
//전투 (Combat) 관련 함수들

void AQPCharacter::AttackPressed()
{
	if (CombatComponent) CombatComponent->StartAttack(); //공격 시작
}
void AQPCharacter::AttackReleased()
{
	if (CombatComponent) CombatComponent->StopAttack(); //공격 멈춤
}

void AQPCharacter::DropPressed()
{
	bDropKeyDown = true; //드랍 키 눌림 상태 설정
	bDropHoldConsumed = false; //드랍 홀드 처리 여부 초기화

	GetWorldTimerManager().ClearTimer(DropHoldTimerHandle); //타이머 초기화
	GetWorldTimerManager().SetTimer(DropHoldTimerHandle, this, &AQPCharacter::OnDropHoldTriggered, DropHoldThreshhold, false); //드랍 홀드 타이
}

void AQPCharacter::DropReleased()
{
	bDropKeyDown = false; //드랍 키 눌림 상태 해제
	GetWorldTimerManager().ClearTimer(DropHoldTimerHandle); //타이머 초기화

	if(bDropHoldConsumed) //드랍 홀드가 처리되지 않은 경우
	{
		bDropHoldConsumed = false; //드랍 홀드 처리 여부 초기화
		return; //함수 종료
	}
}

void AQPCharacter::OnDropHoldTriggered()
{
	if (!bDropKeyDown) return; //드랍 키가 눌리지 않은 경우 함수 종료
	bDropHoldConsumed = true; //드랍 홀드 처리 여부 설정
	TryDropEquipped(); //장착된 아이템 드랍 시도
}

void AQPCharacter::TryDropEquipped()
{
	if (!CombatComponent) return; //전투 컴포넌트가 없으면 함수 종료

	if (CombatComponent->HasWeapon()) {
		CombatComponent->UnEquipWeapon(true); //장착 해제 및 드랍
		return; //함수 종료
	}
}
