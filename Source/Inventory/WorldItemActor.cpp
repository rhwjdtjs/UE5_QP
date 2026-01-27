#include "WorldItemActor.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "PJ_Quiet_Protocol/Character/QPCharacter.h"

AWorldItemActor::AWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemData = nullptr; //아이템 정보 초기화
	Quantity = 1; //아이템 수량 초기화

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent")); //루트 씬 컴포넌트 생성
	SetRootComponent(Root);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere")); //픽업 스피어 컴포넌트 생성
	PickupSphere->SetupAttachment(RootComponent); //루트 컴포넌트
	PickupSphere->SetSphereRadius(PickupSphereRadius); //픽업 스피어 반지름 
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly); //쿼리	전용 충돌 설정
	PickupSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); //모든 채널에 대한 충돌 응답 무시
	PickupSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Overlap); //Pawn 채널에 대해 겹침 응답 설정
	PickupSphere->SetGenerateOverlapEvents(true); //겹침 이벤트 생성 활성화

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
