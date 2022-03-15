#include "WeaponBaseClien.h"

AWeaponBaseClien::AWeaponBaseClien()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetOnlyOwnerSee(true);	// 只对本地用户(拥有者)可见
	
}

void AWeaponBaseClien::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeaponBaseClien::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

