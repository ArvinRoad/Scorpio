#include "WeaponBaseServer.h"

#include "Users/FPSBaseCharacter.h"

AWeaponBaseServer::AWeaponBaseServer()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));	// 碰撞体
	SphereCollision->SetupAttachment(RootComponent);

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);	// 创建 Mesh 的碰撞类型 查询与物理
	WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);

	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);	// 仅查询碰撞
	SphereCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);

	WeaponMesh->SetOwnerNoSee(true);	// 拥有者不可见
	WeaponMesh->SetEnableGravity(true);	// 开始就拥有重力
	WeaponMesh->SetSimulatePhysics(true);	// 初始就设置物理模拟

	SphereCollision->OnComponentBeginOverlap.AddDynamic(this,&AWeaponBaseServer::OnOtherBeginOverlap);		// 当玩家触碰 绑定武器

	/* 初始化武器生成后在客户端复制一个 */
	SetReplicates(true);
}

void AWeaponBaseServer::OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {

	/* 判断碰撞的是不是 FPSBaseCharacter */
	AFPSBaseCharacter* FPSCharacter = Cast<AFPSBaseCharacter>(OtherActor);
	if(FPSCharacter) {
		EquipWeapon();

		/* 玩家逻辑 */
		FPSCharacter->EquipPrimary(this);
	}
}

/* 被拾取时清理自身相关属性 */
void AWeaponBaseServer::EquipWeapon() {
	WeaponMesh->SetEnableGravity(false);			// 卸载重力
	WeaponMesh->SetSimulatePhysics(false);	// 停止模拟
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// 卸载碰撞
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeaponBaseServer::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeaponBaseServer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

