#include "WorldItemActor.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"
#include "Components/SkeletalMeshComponent.h"
AWorldItemActor::AWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemData = nullptr;
	Quantity = 1;
	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh")); //아이템 메쉬 컴포넌트 생성
	ItemMesh->SetupAttachment(RootComponent); //루트 컴포넌트에 부착
	ItemMesh->SetSimulatePhysics(true); //물리 시뮬레이션 활성화
	ItemMesh->SetEnableGravity(true); //중력 활성화
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); //충돌 활성화
	ItemMesh->SetCollisionObjectType(ECC_WorldDynamic); //충돌 객체 타입 설정
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Block); //모든 채널에 블록 응답 설정
	ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); //플레이어 채널에 무시 응답 설정

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere")); //픽업 스피어 컴포넌트 생성
	PickupSphere->SetupAttachment(RootComponent); //루트 컴포넌트에 부착
	PickupSphere->SetSphereRadius(PickupSphereRadius); //픽업 스피어 반지름 설정

	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 쿼리 전용 충돌 설정
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore); // 모든 채널 무시
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 플레이어와 오버랩 설정
	PickupSphere->SetGenerateOverlapEvents(true); // 오버랩 이벤트 생성 활성화



}

void AWorldItemActor::OnPickupBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AQPCharacter* QPCharacter = Cast<AQPCharacter>(OtherActor)) //겹친 액터가 QPCharacter인지 확인
	{
		QPCharacter->SetOverlappingWorldItem(this); //겹치는 월드 아이템 설정
	}
}

void AWorldItemActor::OnPickupEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AQPCharacter* QPCharacter = Cast<AQPCharacter>(OtherActor)) //겹친 액터가 QPCharacter인지 확인
	{
		if (QPCharacter->GetOverlappingWorldItem() == this)
		{
			QPCharacter->SetOverlappingWorldItem(nullptr); //겹치는 월드 아이템 해제
		}
	}
}

void AWorldItemActor::BeginPlay()
{
	Super::BeginPlay();
	//픽업 스피어의 겹침 이벤트 바인딩
	if (PickupSphere)
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AWorldItemActor::OnPickupBegin);
		PickupSphere->OnComponentEndOverlap.AddDynamic(this, &AWorldItemActor::OnPickupEnd);
	}
}
