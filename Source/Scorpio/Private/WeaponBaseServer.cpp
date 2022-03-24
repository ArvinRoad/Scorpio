#include "WeaponBaseServer.h"

#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
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

/* 多播 */
void AWeaponBaseServer::MultShootingEffect_Implementation() {
	/* 先屏蔽掉谁调用进来的 GetOwner() 获取枪的主人 不等于客户端的主人(当前客户端 0 默认的Pawn) */
	if(GetOwner() != UGameplayStatics::GetPlayerPawn(GetWorld(),0)) {
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlash,WeaponMesh,TEXT("Fire_FX_Slot"),FVector::ZeroVector,FRotator::ZeroRotator,FVector::OneVector,EAttachLocation::KeepRelativeOffset,true,EPSCPoolMethod::None,true);	// 枪口特效 EPSCPoolMethod::AutoRelease 表示用完后对池的操作(当前是自动返还到对现池) None|不存储对象池子
		UGameplayStatics::PlaySoundAtLocation(GetWorld(),FireSound,GetActorLocation());	// 在某个位置播放声效
	}
}
bool AWeaponBaseServer::MultShootingEffect_Validate() {
	return true;
}

/* Replicated 宏实现方法，不需要声明父类是AActor 同步服务端和客户端子弹 */
void AWeaponBaseServer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AWeaponBaseServer,ClipCurrentAmmo,COND_None);
}