
#include "ZombieCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AZombieCharacter::AZombieCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	
	bUseControllerRotationPitch = false; //컨트롤러 피치 회전 사용 안함
	bUseControllerRotationRoll = false; //컨트롤러 롤 회전 사용 안함
	bUseControllerRotationYaw = false; //컨트롤러 요 회전 사용 안함

	UCharacterMovementComponent* MoveComponent = GetCharacterMovement(); //캐릭터 무브먼트 컴포넌트 가져오기
	MoveComponent->bOrientRotationToMovement = true; //이동 방향으로 회전 설정
	MoveComponent->RotationRate = FRotator(0.f, 150.f, 0.f); //회전 속도 설정

}

void AZombieCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; //기본 걷기 속도 설정
}

void AZombieCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//GetCharacterMovement()->MaxWalkSpeed = (TargetActor ? ChaseSpeed : WalkSpeed); //타겟이 있으면 추격 속도, 없으면 걷기 속도
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage((uint64)this, 0.f, FColor::Green,
			FString::Printf(TEXT("Target=%s Max=%.1f Walk=%.1f Chase=%.1f"),
				TargetActor ? *TargetActor->GetName() : TEXT("None"),
				GetCharacterMovement()->MaxWalkSpeed, WalkSpeed, ChaseSpeed));
	}
}

void AZombieCharacter::SetTarget(AActor* NewTarget)
{
	TargetActor = NewTarget; //새 타겟 설정
	GetCharacterMovement()->MaxWalkSpeed = (TargetActor ? ChaseSpeed : WalkSpeed); //타겟이 있으면 추격 속도, 없으면 걷기 속도
}

bool AZombieCharacter::CanAttackTarget() const
{
	if (!TargetActor) return false; //타겟이 없으면 공격 불가
	if (bIsAttacking) return false; //이미 공격 중이면 공격 불가

	const float Now = GetWorld()->GetTimeSeconds(); //현재 시간
	if (Now - LastAttackTime < AttackCoolDown) return false; //공격 쿨타임
	const float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation()); //타겟과의 거리
	return DistanceToTarget <= AttackRange; //공격 범위 내에 있는지 확인
	
}
void AZombieCharacter::StartAttack()
{
	if (!CanAttackTarget()) return; //공격 불가 시 함수 종료
	LastAttackTime = GetWorld()->GetTimeSeconds(); //마지막 공격 시간 갱신
	bIsAttacking = true; //공격 상태 설정
	GetCharacterMovement()->StopMovementImmediately(); //즉시 이동 중지 (공격중	에는 이동하지 않도록)
	if(AttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(AttackMontage); //공격 모션 재생
	}
}
