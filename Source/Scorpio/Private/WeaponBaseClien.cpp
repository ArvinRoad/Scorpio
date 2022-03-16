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
	UGameplayStatics::SpawnEmitterAttached(MuzzleFlash,WeaponMesh,TEXT("Fire_FX_Slot"),FVector::ZeroVector,FRotator::ZeroRotator,FVector::OneVector,EAttachLocation::KeepRelativeOffset,true,EPSCPoolMethod::None,true);	// 枪口特效 EPSCPoolMethod::AutoRelease 表示用完后对池的操作(当前是自动返还到对现池) None|不存储对象池子
	UGameplayStatics::PlaySound2D(GetWorld(),FireSound);	// 客户端自身射击声音 所以采用2D
}

