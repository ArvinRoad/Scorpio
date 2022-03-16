#include "Scorpio/Public/Users/FPSBaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

AFPSBaseCharacter::AFPSBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

#pragma region component
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	if(PlayerCamera) {
		PlayerCamera->SetupAttachment(RootComponent);
		PlayerCamera->bUsePawnControlRotation = true;	// 使用控制器调节摄像头上下移动
	}

	FPArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmsMesh"));
	if(FPArmsMesh) {
		FPArmsMesh->SetupAttachment(PlayerCamera);
		FPArmsMesh->SetOnlyOwnerSee(true);	// 仅自己可
	}

	Mesh->SetOwnerNoSee(true);	// 拥有者不可见

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
#pragma endregion 

}

void AFPSBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	StartWithKindOfWeapon();	// 购买枪支(沙漠之鹰)
}

void AFPSBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFPSBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	InputComponent->BindAction(TEXT("LowSpeedWalk"),IE_Pressed,this,&AFPSBaseCharacter::LowSpeedWalkAction);
	InputComponent->BindAction(TEXT("LowSpeedWalk"),IE_Released,this,&AFPSBaseCharacter::NormalSpeedWalkAction);
	InputComponent->BindAction(TEXT("Jump"),IE_Pressed,this,&AFPSBaseCharacter::JumpAction);
	InputComponent->BindAction(TEXT("Jump"),IE_Released,this,&AFPSBaseCharacter::StopJumpAction);
	InputComponent->BindAction(TEXT("Fire"),IE_Pressed,this,&AFPSBaseCharacter::InputFirePressed);
	InputComponent->BindAction(TEXT("Fire"),IE_Released,this,&AFPSBaseCharacter::InputFireReleased);
	
	InputComponent->BindAxis(TEXT("MoveForward"),this,&AFPSBaseCharacter::MoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"),this,&AFPSBaseCharacter::MoveRight);
	InputComponent->BindAxis(TEXT("Turn"),this,&AFPSBaseCharacter::AddControllerYawInput);
	InputComponent->BindAxis(TEXT("LookUp"),this,&AFPSBaseCharacter::AddControllerPitchInput);
	
}

#pragma region NetWorking
/* 静步服务器同步 */
void AFPSBaseCharacter::ServerLowSpeedWalkAction_Implementation() {
	CharacterMovement->MaxWalkSpeed = 300;
}
bool AFPSBaseCharacter::ServerLowSpeedWalkAction_Validate() {
	return true;
}
void AFPSBaseCharacter::ServerNormalSpeedWalkAction_Implementation() {
	CharacterMovement->MaxWalkSpeed = 600;
}
bool AFPSBaseCharacter::ServerNormalSpeedWalkAction_Validate() {
	return true;
}

/* 动态创建第一人称客户端武器 服务器下发客户端 服务器不需要生成 */
void AFPSBaseCharacter::ClientEquipFPArmsPrimary_Implementation() {
	if(ServerPrimaryWeapon) {
		/* 如果客户端以及有了 那就不用创建了 */
		if(ClientPrimaryWeapon) {
			
		}else {
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner = this;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ClientPrimaryWeapon = GetWorld()->SpawnActor<AWeaponBaseClien>(ServerPrimaryWeapon->ClientWeaponBaseBPClass,GetActorTransform(),SpawnInfo);
			ClientPrimaryWeapon->K2_AttachToComponent(FPArmsMesh,TEXT("WeaponSocket"),EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);

			/* 手臂动画 */
		}
	}
}

#pragma endregion 

/* 键盘输入事件 */
#pragma region InputEvent
void AFPSBaseCharacter::MoveForward(float AxisValue) {
	AddMovementInput(GetActorForwardVector(),AxisValue,false);
}
void AFPSBaseCharacter::MoveRight(float AxisValue) {
	AddMovementInput(GetActorRightVector(),AxisValue,false);
}

void AFPSBaseCharacter::JumpAction() {
	Jump();
}
void AFPSBaseCharacter::StopJumpAction() {
	StopJumping();
}
void AFPSBaseCharacter::InputFirePressed() {
	/* 根据当前武器类型选择方法 */
	switch(ActiveWeapon) {
		case EWeaponType::AK47: {
				FireWeaponPrimary();
			}
	}
}
void AFPSBaseCharacter::InputFireReleased() {
	/* 根据当前武器类型选择方法 */
	switch (ActiveWeapon) {
		case EWeaponType::AK47: {
				StopFirePrimary();
			}
	}
	
}
void AFPSBaseCharacter::LowSpeedWalkAction() {
	CharacterMovement->MaxWalkSpeed = 300;
	ServerLowSpeedWalkAction();
}
void AFPSBaseCharacter::NormalSpeedWalkAction() {
	CharacterMovement->MaxWalkSpeed = 600;
	ServerNormalSpeedWalkAction();
}

#pragma endregion

#pragma region Weapon
/* 玩家装备武器 */
void AFPSBaseCharacter::EquipPrimary(AWeaponBaseServer* WeaponBaseServer) {
	if(ServerPrimaryWeapon) {
		
	}else {
		ServerPrimaryWeapon = WeaponBaseServer;
		ServerPrimaryWeapon->SetOwner(this);
		ServerPrimaryWeapon->K2_AttachToComponent(Mesh,TEXT("Weapon_Rifle"),EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);	// 添加到第三人称上
		ClientEquipFPArmsPrimary();	// 让客户端去生成
	}
}

/* 开局自带枪 */
void AFPSBaseCharacter::StartWithKindOfWeapon() {
	/* 判断当前代码对人物是否有主控权，有主控权就是在服务器下发的指令 如果是服务器就购买武器 */
	if(HasAuthority()) {
		PurchaseWeapon(EWeaponType::AK47);	// 开局枪类型
	}
}

void AFPSBaseCharacter::PurchaseWeapon(EWeaponType WeaponType) {
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = this;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	switch (WeaponType) {
		case EWeaponType::AK47: {
				/* 动态获取AK47 Server类 */
				UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/_Scorpio/Blueprint/Weapon/AK47/ServerBP_AK47.ServerBP_AK47_C'"));	// 引用后加上_C才能获取(代表一个Class)
				AWeaponBaseServer* ServerWeapon =  GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,GetActorTransform(),SpawnInfo);
				ServerWeapon->EquipWeapon();	// 主动卸载碰撞
				EquipPrimary(ServerWeapon);
			}
			break;
		default: {
				
			}
	}
}
#pragma endregion 

/* 换弹与射击相关 */
#pragma region Fire
void AFPSBaseCharacter::FireWeaponPrimary() {
	UE_LOG(LogTemp,Warning,TEXT("射击中:void AFPSBaseCharacter::FireWeaponPrimary()"));
	// 服务端：减少弹药 | 射线检测 (三种) | 伤害应用 | 弹孔生成

	// 客户端：枪体动画 | 手臂动画 | 射击声效 | 屏幕抖动 | 后作力 | 枪口特效

	// 射击模式：连发 | 单射 | 点发
}
void AFPSBaseCharacter::StopFirePrimary() {
	// 析构FireWeaponPrimary 参数
}
#pragma endregion