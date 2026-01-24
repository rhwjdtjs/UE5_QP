#include "QPCombatComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "PJ_Quiet_Protocol/Weapons/WeaponBase.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

#define TRACE_LENGTH 80000.f //충돌 검사 거리

UQPCombatComponent::UQPCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UQPCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner()); //소유한 캐릭터 가져오기
}

bool UQPCombatComponent::EquipWeapon(AWeaponBase* NewWeapon, bool bUnequipCurrent)
{
	if (!OwnerCharacter || !NewWeapon) return false; //소유한 캐릭터나 새 무기가 유효하지 않으면 false 반환
	if (EquippedWeapon == NewWeapon) return true; //이미 장착된 무기라면 true 반환
	if (EquippedWeapon && bUnequipCurrent) { //현재 무기가 있고 해제 플래그가 true라면
		UnEquipWeapon(true); //현재 무기 해제
	}
	EquippedWeapon = NewWeapon; //새 무기 장착
	//소유자/충돌 기본 처리 (**1 나중에 확장)
	EquippedWeapon->SetOwner(OwnerCharacter);//소유자 설정
	EquippedWeapon->SetInstigator(Cast<APawn>(OwnerCharacter)); //인스티게이터 설정
	EquippedWeapon->SetActorEnableCollision(false); //충돌 비활성화
	EquippedWeapon->OnEquipped(OwnerCharacter); //무기 장착 처리 호출
	if (!AttachWeaponToCharacter(EquippedWeapon)) //캐릭터에 무기 부착 실패 시
	{
		EquippedWeapon->OnUnequipped(true); //무기 해제 처리 호출
		EquippedWeapon = nullptr; //장착 실패 시 무기 초기화
		SetWeaponType(EQPWeaponType::EWT_None); //무기 타입 없음으로 설정
		return false; //false 반환
	}
	SetWeaponType(NewWeapon->GetWeaponType()); //무기 타입 설정 //**2 WeaponBase에서 GetWeaponType() 구현 필요
	return true; //성공적으로 장착했으므로 true 반환
}

bool UQPCombatComponent::UnEquipWeapon(bool bDropToWorld)
{
	if (!OwnerCharacter || !EquippedWeapon) {
		SetWeaponType(EQPWeaponType::EWT_None); //무기 타입 없음으로 설정
		return false; //소유한 캐릭터나 장착된 무기가 없으면 false 반환
	}
	StopAttack(); //공격 중지
	//캐릭터 분리
	EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); //월드 트랜스폼 유지하며 분리
	EquippedWeapon->OnUnequipped(bDropToWorld); //무기 해제 처리 호출
	EquippedWeapon = nullptr; //장착된 무기 초기화

	SetWeaponType(EQPWeaponType::EWT_None); //무기 타입 없음으로 설정
	return true; //성공적으로 해제했으므로 true 반환
}
void UQPCombatComponent::StartAttack()
{
	if (!EquippedWeapon) {
		SetIsAttacking(false); //공격 상태 false로 설정
		return; //장착된 무기가 없으면 반환
	}
	SetIsAttacking(true); //공격 상태 true로 설정
	EquippedWeapon->StartFire(); //무기 발사 시작

	if (AQPCharacter* QPChar = Cast<AQPCharacter>(OwnerCharacter)) //애니메이션 재생
	{
		QPChar->PlayFireMontage(bIsAiming); //조준 상태에 따라 발사 몽타주 재생
	}

}
void UQPCombatComponent::StopAttack()
{
	if (EquippedWeapon) {
		EquippedWeapon->StopAttack(); //무기 공격 중지
	}
	SetIsAttacking(false); //공격 상태 false로 설정
}
void UQPCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult HitResult; //충돌 결과 변수
	TraceUnderCrosshairs(HitResult); //조준선 아래 충돌 검사
}

bool UQPCombatComponent::AttachWeaponToCharacter(AWeaponBase* Weapon)
{
	if (!OwnerCharacter || !Weapon) return false; //소유한 캐릭터나 무기가 유효하지 않으면 false 반환
	USkeletalMeshComponent* MeshComponent = OwnerCharacter->GetMesh(); //캐릭터의 스켈레탈 메쉬 컴포넌트 가져오기
	if (!MeshComponent) return false; //메쉬 컴포넌트가 유효하지 않으면 false 반환
	Weapon->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, EquipSocketName); //무기를 캐릭터 메쉬에 부착
	return true; //성공적으로 부착했으므로 true 반환
}

void UQPCombatComponent::SetWeaponType(EQPWeaponType NewType)
{
	if (EquippedWeaponType == NewType) return; //이미 같은 타입이면 반환
	EquippedWeaponType = NewType; //무기 타입 설정
	OnWeaponTypeChanged.Broadcast(EquippedWeaponType); //무기 타입 변경 델리게이트 브로드캐스트
}
void UQPCombatComponent::SetIsAttacking(bool bNewIsAttacking)
{
	if (bIsAttacking == bNewIsAttacking) return; //이미 같은 상태이면 반환
	bIsAttacking = bNewIsAttacking; //공격 상태 설정
	OnAttackStateChanged.Broadcast(bIsAttacking); //공격 상태 변경 델리게이트 브로드캐스트
}

void UQPCombatComponent::SetAiming(bool bNewAiming) //조준 상태 설정 함수
{
	if (bIsAiming == bNewAiming) return; //이미 같은 상태이면 반환

	SetIsAiming(bNewAiming);
}

void UQPCombatComponent::SetIsAiming(bool bNewIsAiming)
{
	bIsAiming = bNewIsAiming; //조준 상태 설정

	OnAimStateChanged.Broadcast(bIsAiming); //조준 상태 변경 델리게이트 브로드캐스트
}

void UQPCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize; //뷰포트 크기 변수
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize); //뷰포트 크기 가져오기
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f); //화면 중앙 위치 계산
	FVector CorsshairWorldLocation, CorsshairWorldDirection; //월드 위치 및 방향 변수
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0), //플레이어 컨트롤러 가져오기
		CrosshairLocation, //스크린 위치
		CorsshairWorldLocation, //월드 위치 출력
		CorsshairWorldDirection //월드 방향 출력
	);

	if (bScreenToWorld) //스크린을 월드로 변환 성공 시
	{
		const FVector Start = CorsshairWorldLocation; //시작 위치 설정
		const FVector End = Start + (CorsshairWorldDirection * TRACE_LENGTH); //끝 위치 설정 (80,000 유닛 앞)
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult, //히트 결과 출력
			Start, //시작 위치
			End, //끝 위치
			ECollisionChannel::ECC_Visibility //가시성 채널 사용
		);

		if (!TraceHitResult.bBlockingHit) //충돌이 없으면
		{
			TraceHitResult.ImpactPoint = End; //충돌 지점을 끝 위치로 설정
		}
		else
		{
			//충돌 지점에 디버그 점 그리기 (개발용)
			DrawDebugSphere(
				GetWorld(),
				TraceHitResult.ImpactPoint, //충돌 지점
				16.f, //반지름
				12, //세그먼트 수
				FColor::Red, //색상
				false, //영구적이지 않음
				0.f //지속 시간
			);

		}
	}

}