#include "WeaponBaseClien.h"

#include "Kismet/GameplayStatics.h"

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

void AWeaponBaseClien::DisplayWeaponEffect() {
	UGameplayStatics::PlaySound2D(GetWorld(),FireSound);	// 客户端自身射击声音 所以采用2D
}

