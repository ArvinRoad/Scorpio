#include "Scorpio/Public/Users/FPSBaseCharacter.h"

#include "Blueprint/UserWidget.h"
#include "Components/DecalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

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

#pragma region Engine
void AFPSBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = 100; // 初始生命100
	IsFiring = false;	// 是否在射击
	IsReloading = false;	// 是否在换弹
	IsAiming = false;	// 狙击枪开镜
	OnTakePointDamage.AddDynamic(this,&AFPSBaseCharacter::OnHit);	// 伤害回调
	StartWithKindOfWeapon();	// 购买枪支(沙漠之鹰)
	ClientArmsAnimBP = FPArmsMesh->GetAnimInstance();	// 手臂获取动画初始化
	ServerBodysAnimBP = Mesh->GetAnimInstance();	// 射击全身动画初始化
	FPSPlayerController = Cast<AFPSPlayerController>(GetController());	// 持有AFPSPlayerController类型指针 屏幕抖动
	if(FPSPlayerController) {
		FPSPlayerController->CreatePlayerUI();
	}
}

/* Replicated 宏实现方法，不需要声明父类是AActor 同步服务端和客户端射击状态 */
void AFPSBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AFPSBaseCharacter,IsFiring,COND_None);
	DOREPLIFETIME_CONDITION(AFPSBaseCharacter,IsReloading,COND_None);
	DOREPLIFETIME_CONDITION(AFPSBaseCharacter,ActiveWeapon,COND_None);
	DOREPLIFETIME_CONDITION(AFPSBaseCharacter,IsAiming,COND_None);
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
	InputComponent->BindAction(TEXT("Reload"),IE_Pressed,this,&AFPSBaseCharacter::InputReload);
	InputComponent->BindAction(TEXT("Aiming"),IE_Pressed,this,&AFPSBaseCharacter::InputAimingPressed);
	InputComponent->BindAction(TEXT("Aiming"),IE_Released,this,&AFPSBaseCharacter::InputAimingReleased);
	
	InputComponent->BindAxis(TEXT("MoveForward"),this,&AFPSBaseCharacter::MoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"),this,&AFPSBaseCharacter::MoveRight);
	InputComponent->BindAxis(TEXT("Turn"),this,&AFPSBaseCharacter::AddControllerYawInput);
	InputComponent->BindAxis(TEXT("LookUp"),this,&AFPSBaseCharacter::AddControllerPitchInput);
}
#pragma endregion 

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
void AFPSBaseCharacter::ServerFireRifleWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,bool IsMoving) {
	if(ServerPrimaryWeapon) {
		/* 服务端逻辑对标：ClientFire_Implementation 多播(必须在服务器调用 | 什么调用什么多播) */
		ServerPrimaryWeapon->MultShootingEffect();
		ServerPrimaryWeapon->ClipCurrentAmmo -= 1;	// 开枪减少子弹
		MultShooting(); // 多播 身体射击动画蒙太奇
		ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);	// 弹药更新
		//UKismetSystemLibrary::PrintString(this,FString::Printf(TEXT("ServerPrimaryWeapon->ClipCurrentAmmo: %d"),ServerPrimaryWeapon->ClipCurrentAmmo)); // DeBug输出子弹数
	}
	IsFiring = true;	// 正在射击
	RifleLineTrace(CameraLocation,CameraRotation,IsMoving);	// 射线检测
}
bool AFPSBaseCharacter::ServerFireRifleWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving) {
	return true;
}
void AFPSBaseCharacter::ServerFireSniperWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,bool IsMoving) {
	if (ServerPrimaryWeapon){
		/* 多播 必须在服务器调用 谁调谁多播 */
		ServerPrimaryWeapon->MultShootingEffect();
		ServerPrimaryWeapon->ClipCurrentAmmo -= 1;
		/* 多播身体射击动画 */
		MultShooting();
		/* 客户端UI更新 */
		ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);
	}

	if(ClientPrimaryWeapon) {
		/* 计时器：重置后坐力算法参数 */
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;	// 执行者
		ActionInfo.ExecutionFunction = TEXT("DelaySniperShootCallBack");	// 执行的方法
		ActionInfo.UUID = FMath::Rand();	// Action 的ID
		ActionInfo.Linkage = 0;	// 不能为-1,否则无法执行
		UKismetSystemLibrary::Delay(this,ClientPrimaryWeapon->ClientArmsFireAnimMontage->GetPlayLength(),ActionInfo);
		
	}
	
	IsFiring = true;
	SniperLineTrace(CameraLocation,CameraRotation,IsMoving);
}
bool AFPSBaseCharacter::ServerFireSniperWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving) {
	return true;
}
void AFPSBaseCharacter::ServerFirePistolWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,bool IsMoving) {
	if(ServerSecondaryWeapon) {
		/* 计时器：重置后坐力算法参数 */
		FLatentActionInfo ActionInfo;
		ActionInfo.CallbackTarget = this;	// 执行者
		ActionInfo.ExecutionFunction = TEXT("DelaySpreadWeaponShootCallBack");	// 执行的方法
		ActionInfo.UUID = FMath::Rand();	// Action 的ID
		ActionInfo.Linkage = 0;	// 不能为-1,否则无法执行
		UKismetSystemLibrary::Delay(this,ServerSecondaryWeapon->SpreadWeaponCallBackRate,ActionInfo);
		//UKismetSystemLibrary::PrintString(this,FString::Printf(TEXT("ServerFirePistolWeapon_Implementation")));	// 测试换弹是否成功日志
		
		/* 服务端逻辑对标：ClientFire_Implementation 多播(必须在服务器调用 | 什么调用什么多播) */
		ServerSecondaryWeapon->MultShootingEffect();
		ServerSecondaryWeapon->ClipCurrentAmmo -= 1;	// 开枪减少子弹
		MultShooting(); // 多播 身体射击动画蒙太奇
		ClientUpdateAmmoUI(ServerSecondaryWeapon->ClipCurrentAmmo,ServerSecondaryWeapon->GunCurrentAmmo);	// 弹药更新
		//UKismetSystemLibrary::PrintString(this,FString::Printf(TEXT("ServerPrimaryWeapon->ClipCurrentAmmo: %d"),ServerPrimaryWeapon->ClipCurrentAmmo)); // DeBug输出子弹数
	}
	IsFiring = true;	// 正在射击
	PistolLineTrace(CameraLocation,CameraRotation,IsMoving);	// 射线检测
}
bool AFPSBaseCharacter::ServerFirePistolWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving) {
	return true;
}

/* 换弹方法 */
void AFPSBaseCharacter::ServerReloadPrimary_Implementation() {
	if(ServerPrimaryWeapon) {
		if(ServerPrimaryWeapon->GunCurrentAmmo > 0 && ServerPrimaryWeapon->ClipCurrentAmmo < ServerPrimaryWeapon->MaxClipAmmo) {
			/* 客户端: 手臂动画 | 数据更新 | UI更新 */
			ClientReload();
			/* 服务器：多播身体动画 | 数据更新 | UI更新 */
			MultiReloadAnimation();
			IsReloading = true;
			// 延迟
			if(ClientPrimaryWeapon) {
				FLatentActionInfo ActionInfo;
				ActionInfo.CallbackTarget =	this;	// 执行者
				ActionInfo.ExecutionFunction = TEXT("DelayPlayArmReloadCallBack");	// 执行的方法
				ActionInfo.UUID = FMath::Rand();	// Action 的ID
				ActionInfo.Linkage = 0;	// 不能为-1,否则无法执行
				UKismetSystemLibrary::Delay(this,ClientPrimaryWeapon->ClientArmsReloadAnimMontage->GetPlayLength(),ActionInfo);
				//UKismetSystemLibrary::PrintString(this,FString::Printf(TEXT("ServerReloadPrimary_Implementation"))); // 测试换弹是否成功日志
			}
		}
	}
}
bool AFPSBaseCharacter::ServerReloadPrimary_Validate() {
	return true;
}
void AFPSBaseCharacter::ServerReloadSecondary_Implementation() {
	if(ServerSecondaryWeapon) {
		if(ServerSecondaryWeapon->GunCurrentAmmo > 0 && ServerSecondaryWeapon->ClipCurrentAmmo < ServerSecondaryWeapon->MaxClipAmmo) {
			/* 客户端: 手臂动画 | 数据更新 | UI更新 */
			ClientReload();
			/* 服务器：多播身体动画 | 数据更新 | UI更新 */
			MultiReloadAnimation();
			IsReloading = true;
			// 延迟
			if(ClientSecondaryWeapon) {
				FLatentActionInfo ActionInfo;
				ActionInfo.CallbackTarget =	this;	// 执行者
				ActionInfo.ExecutionFunction = TEXT("DelayPlayArmReloadCallBack");	// 执行的方法
				ActionInfo.UUID = FMath::Rand();	// Action 的ID
				ActionInfo.Linkage = 0;	// 不能为-1,否则无法执行
				UKismetSystemLibrary::Delay(this,ClientSecondaryWeapon->ClientArmsReloadAnimMontage->GetPlayLength(),ActionInfo);
				//UKismetSystemLibrary::PrintString(this,FString::Printf(TEXT("ServerReloadPrimary_Implementation"))); // 测试换弹是否成功日志
			}
		}
	}
}
bool AFPSBaseCharacter::ServerReloadSecondary_Validate() {
	return true;
}

void AFPSBaseCharacter::ServerStopFiring_Implementation() {
	IsFiring = false;	// 非射击状态
}
bool AFPSBaseCharacter::ServerStopFiring_Validate() {
	return true;
}

void AFPSBaseCharacter::ServerSetAiming_Implementation(bool AimingState) {
	IsAiming = AimingState;
}
bool AFPSBaseCharacter::ServerSetAiming_Validate(bool AimingState) {
	return true;
}

void AFPSBaseCharacter::MultShooting_Implementation() {
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodysWeaponAtcor();
	if(ServerBodysAnimBP) {
		if(CurrentServerWeapon) {
			ServerBodysAnimBP->Montage_Play(CurrentServerWeapon->ServerTPBodysShootAnimMontage);
		}
	}
}
bool AFPSBaseCharacter::MultShooting_Validate() {
	return true;
}
void AFPSBaseCharacter::MultiReloadAnimation_Implementation() {
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodysWeaponAtcor();
	if(ServerBodysAnimBP) {
		if(CurrentServerWeapon) {
			ServerBodysAnimBP->Montage_Play(CurrentServerWeapon->ServerTpBodysReloadAnimMontage);
			CurrentServerWeapon->ServerPlayReloadAnimation();
		}
	}
}
bool AFPSBaseCharacter::MultiReloadAnimation_Validate() {
	return true;
}

void AFPSBaseCharacter::MultiSpawnBulletDecal_Implementation(FVector Location,FRotator Rotation) {
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodysWeaponAtcor();
	if(CurrentServerWeapon) {
		UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(GetWorld(),CurrentServerWeapon->BulletDecalMaterial,FVector(8,8,8),Location,Rotation,10);	// 生成弹孔
		if(Decal) {
			Decal->SetFadeScreenSize(0.001);	// 修改弹孔可见距离
		}
	}
}
bool AFPSBaseCharacter::MultiSpawnBulletDecal_Validate(FVector Location,FRotator Rotation) {
	return true;
}
void AFPSBaseCharacter::ClientAiming_Implementation() {
	// 瞄准镜的UI 关闭枪体可见 摄像头距离拉远 客户端RPC
	if(ClientPrimaryWeapon) {
		ClientPrimaryWeapon->SetActorHiddenInGame(true);
		if (FPArmsMesh) {
			FPArmsMesh->SetHiddenInGame(true);
		}
		if(PlayerCamera) {
			PlayerCamera->SetFieldOfView(ClientPrimaryWeapon->FieldOfAimingView);
		}
	}
	WidgetScope = CreateWidget<UUserWidget>(GetWorld(),SniperScopeBPClass);
	WidgetScope->AddToViewport(); // 直接加载到显示器上
}
void AFPSBaseCharacter::ClientEndAiming_Implementation() {
	if(ClientPrimaryWeapon) {
		ClientPrimaryWeapon->SetActorHiddenInGame(false);
		if(FPArmsMesh) {
			FPArmsMesh->SetHiddenInGame(false);
		}
		if (PlayerCamera) {
			PlayerCamera->SetFieldOfView(90); // 默认值为90
		}
	}
	if(WidgetScope) {
		WidgetScope->RemoveFromParent();
	}
}

void AFPSBaseCharacter::ClientReload_Implementation() {
	/* 手臂换弹蒙太奇动画 */
	AWeaponBaseClien* CurrentClientWeapon = GetCurrentClientFPArmsWeaponAction();
	if(CurrentClientWeapon) {
		UAnimMontage* ClientArmsReloadMontage = CurrentClientWeapon->ClientArmsReloadAnimMontage;
		ClientArmsAnimBP->Montage_Play(ClientArmsReloadMontage);
		CurrentClientWeapon->PlayReloadAnimation();
	}
}

void AFPSBaseCharacter::ClientRecoil_Implementation() {
	UCurveFloat* VerticalRecoilCurve = nullptr;
	UCurveFloat* HorizontalRecoilCurve = nullptr;
	if(ServerPrimaryWeapon) {
		VerticalRecoilCurve = ServerPrimaryWeapon->VerticalRecoilCurve;
		HorizontalRecoilCurve = ServerPrimaryWeapon->HorizontalRecoilCurve;
	}
	RecoilXCoordPerShoot += 0.1;
	if(VerticalRecoilCurve) {
		NewVerticalRecoilAmount = VerticalRecoilCurve->GetFloatValue(RecoilXCoordPerShoot);
	}
	if(HorizontalRecoilCurve) {
		NewHorizontalRecoilAmount = HorizontalRecoilCurve->GetFloatValue(RecoilXCoordPerShoot);
	}
	VerticalRecoilAmount = NewVerticalRecoilAmount - OldVerticalRecoilAmount;
	HorizontalRecoilAmount = NewHorizontalRecoilAmount - OldHorizontalRecoilAmount;
	if(FPSPlayerController) {
		FRotator ControllerRotator = FPSPlayerController->GetControlRotation();
		FPSPlayerController->SetControlRotation(FRotator(ControllerRotator.Pitch + VerticalRecoilAmount,ControllerRotator.Yaw + HorizontalRecoilAmount,ControllerRotator.Roll));
	}
	OldVerticalRecoilAmount = NewVerticalRecoilAmount;
	OldHorizontalRecoilAmount = NewHorizontalRecoilAmount;
}
void AFPSBaseCharacter::ClientUpdateHealthUI_Implementation(float NewHealth) {
	if(FPSPlayerController) {
		FPSPlayerController->UpdateHealthUI(NewHealth);
	}
}
void AFPSBaseCharacter::ClientUpdateAmmoUI_Implementation(int32 ClipCurrentAmmo, int32 GunCurrentAmmo) {
	if(FPSPlayerController) {
		FPSPlayerController->UpdateAmmoUI(ClipCurrentAmmo,GunCurrentAmmo);
	}
}
void AFPSBaseCharacter::ClientFire_Implementation() {
	AWeaponBaseClien* CurrentClientWeapon = GetCurrentClientFPArmsWeaponAction();
	if(CurrentClientWeapon) {
		/* 枪体动画 */
		CurrentClientWeapon->PlayShootAnimation();
		
		/* 手臂的播放动画 蒙太奇 */
		UAnimMontage* ClientArmsFireMontage = CurrentClientWeapon->ClientArmsFireAnimMontage;
		ClientArmsAnimBP->Montage_SetPlayRate(ClientArmsFireMontage,1);	// 蒙太奇动画速率 1 倍速
		ClientArmsAnimBP->Montage_Play(ClientArmsFireMontage);

		/* 射击声效 */
		CurrentClientWeapon->DisplayWeaponEffect();

		/* 屏幕抖动 */
		FPSPlayerController->PlayerCameraShake(CurrentClientWeapon->CameraShakeClass);

		/* 十字线UI扩散动画 */
		FPSPlayerController->DoCrosshairRecoil();
	}
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
			FName WeaponSocketName = TEXT("WeaponSocket");
			if(ActiveWeapon == EWeaponType::M4A1) {
				WeaponSocketName = TEXT("M4A1_Socket");
			}
			if (ActiveWeapon == EWeaponType::Sniper) {
				WeaponSocketName = TEXT("AWP_Socket");
			}
			ClientPrimaryWeapon->K2_AttachToComponent(FPArmsMesh,WeaponSocketName,EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);	// 不同武器插槽不一样
			ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);	// 弹药更新

			/* 手臂动画混合 */
			if(ClientPrimaryWeapon) {
				UpdateFPArmsBlendPose(ClientPrimaryWeapon->FPArmsBlendPose);
			}
		}
	}
}
void AFPSBaseCharacter::ClientEquipFPArmsSecondary_Implementation() {
	if(ServerSecondaryWeapon) {
		/* 如果客户端以及有了 那就不用创建了 */
		if(ClientSecondaryWeapon) {
			
		}else {
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner = this;
			SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ClientSecondaryWeapon = GetWorld()->SpawnActor<AWeaponBaseClien>(ServerSecondaryWeapon->ClientWeaponBaseBPClass,GetActorTransform(),SpawnInfo);
			FName WeaponSocketName = TEXT("WeaponSocket");
			ClientSecondaryWeapon->K2_AttachToComponent(FPArmsMesh,WeaponSocketName,EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);	// 不同武器插槽不一样
			ClientUpdateAmmoUI(ServerSecondaryWeapon->ClipCurrentAmmo,ServerSecondaryWeapon->GunCurrentAmmo);	// 弹药更新

			/* 手臂动画混合 */
			if(ClientSecondaryWeapon) {
				UpdateFPArmsBlendPose(ClientSecondaryWeapon->FPArmsBlendPose);
			}
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
		case EWeaponType::M4A1: {
				FireWeaponPrimary();
			}
		case EWeaponType::MP7: {
				FireWeaponPrimary();
			}
		case EWeaponType::DesertEagle: {
				FireWeaponSecondary();
			}
		case EWeaponType::Sniper: {
				FireWeaponSniper();
			}
	}
}
void AFPSBaseCharacter::InputFireReleased() {
	/* 根据当前武器类型选择方法 */
	switch (ActiveWeapon) {
		case EWeaponType::AK47: {
				StopFirePrimary();
			}
		case EWeaponType::M4A1: {
				StopFirePrimary();
			}
		case EWeaponType::MP7: {
				StopFirePrimary();
			}
		case EWeaponType::DesertEagle: {
				StopFireSecondary();
			}
		case EWeaponType::Sniper: {
				StopFireSniper();
			}
	}
	
}

void AFPSBaseCharacter::InputAimingPressed() {
	if(ActiveWeapon == EWeaponType::Sniper) {
		// 更改 IsAiming 服务器端RPC
		ServerSetAiming(true);
		// 瞄准镜的UI 关闭枪体可见 摄像头距离拉远 客户端RPC
		ClientAiming();
	}
}
void AFPSBaseCharacter::InputAimingReleased() {
	if(ActiveWeapon == EWeaponType::Sniper) {
		// 更改 IsAiming 服务器端RPC
		ServerSetAiming(false);
		// 删除瞄准镜UI 开启枪体可见 摄像头距离恢复
		ClientEndAiming();
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
void AFPSBaseCharacter::InputReload() {
	if(!IsReloading) {
		if(!IsFiring) {
			switch (ActiveWeapon) {
				case EWeaponType::AK47: {
						ServerReloadPrimary();
					}
				case EWeaponType::M4A1: {
						ServerReloadPrimary();
					}
				case EWeaponType::MP7: {
						ServerReloadPrimary();
					}
				case EWeaponType::DesertEagle: {
						ServerReloadSecondary();
					}
				case EWeaponType::Sniper: {
						ServerReloadPrimary();
					}
			}
		}
	}
}

#pragma endregion

#pragma region Weapon
/* 玩家装备主武器 */
void AFPSBaseCharacter::EquipPrimary(AWeaponBaseServer* WeaponBaseServer) {
	if(ServerPrimaryWeapon) {
		
	}else {
		ServerPrimaryWeapon = WeaponBaseServer;
		ServerPrimaryWeapon->SetOwner(this);
		ServerPrimaryWeapon->K2_AttachToComponent(Mesh,TEXT("Weapon_Rifle"),EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);	// 添加到第三人称上
		ClientEquipFPArmsPrimary();	// 让客户端去生成
	}
}

/* 玩家装备副武器 */
void AFPSBaseCharacter::EquipSecondary(AWeaponBaseServer* WeaponBaseServer) {
	if(ServerSecondaryWeapon) {
		
	}else {
		ServerSecondaryWeapon = WeaponBaseServer;
		ServerSecondaryWeapon->SetOwner(this);
		ServerSecondaryWeapon->K2_AttachToComponent(Mesh,TEXT("Weapon_Rifle"),EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);	// 添加到第三人称上
		ClientEquipFPArmsSecondary();	// 让客户端去生成
	}
}

/* 开局自带枪 */
void AFPSBaseCharacter::StartWithKindOfWeapon() {
	/* 判断当前代码对人物是否有主控权，有主控权就是在服务器下发的指令 如果是服务器就购买武器 */
	if(HasAuthority()) {
		PurchaseWeapon(TestStartWeapon);	// 开局枪类型
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
				ActiveWeapon = EWeaponType::AK47;
				EquipPrimary(ServerWeapon);
			}
			break;
		case EWeaponType::M4A1: {
				/* 动态获取M4A1 Server类 */
				UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/_Scorpio/Blueprint/Weapon/M4A1/ServerBP_M4A1.ServerBP_M4A1_C'"));
				AWeaponBaseServer* ServerWeapon = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,GetActorTransform(),SpawnInfo);
				ServerWeapon->EquipWeapon();
				ActiveWeapon = EWeaponType::M4A1;
				EquipPrimary(ServerWeapon);
			}
			break;
		case EWeaponType::MP7: {
				/* 动态获取MP7 Server类 */
				UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/_Scorpio/Blueprint/Weapon/MP7/ServerBP_MP7.ServerBP_MP7_C'"));
				AWeaponBaseServer* ServerWeapon = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,GetActorTransform(),SpawnInfo);
				ServerWeapon->EquipWeapon();
				ActiveWeapon = EWeaponType::MP7;
				EquipPrimary(ServerWeapon);
			}
			break;
		case EWeaponType::DesertEagle: {
				/* 动态获取DesertEagle Server类 */
				UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/_Scorpio/Blueprint/Weapon/DesertEagle/ServerBP_DesertEagle.ServerBP_DesertEagle_C'"));
				AWeaponBaseServer* ServerWeapon = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,GetActorTransform(),SpawnInfo);
				ServerWeapon->EquipWeapon();
				ActiveWeapon = EWeaponType::DesertEagle;
				EquipSecondary(ServerWeapon);
			}
		case EWeaponType::Sniper: {
				/* 动态获取Sniper Server类 */
				UClass* BlueprintVar = StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/_Scorpio/Blueprint/Weapon/Sniper/ServerBP_Sniper.ServerBP_Sniper_C'"));
				AWeaponBaseServer* ServerWeapon = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,GetActorTransform(),SpawnInfo);
				ServerWeapon->EquipWeapon();
				ActiveWeapon = EWeaponType::Sniper;
				EquipPrimary(ServerWeapon);
			}
			break;
		default: {
				
			}
	}
}

AWeaponBaseClien* AFPSBaseCharacter::GetCurrentClientFPArmsWeaponAction(){
	switch(ActiveWeapon) {
		case EWeaponType::AK47: {
				return ClientPrimaryWeapon;
			}
		case EWeaponType::M4A1: {
				return ClientPrimaryWeapon;
			}
		case EWeaponType::MP7: {
				return ClientPrimaryWeapon;
			}
		case EWeaponType::DesertEagle: {
				return ClientSecondaryWeapon;
			}
		case EWeaponType::Sniper: {
				return ClientPrimaryWeapon;
			}
	}
	return nullptr;
}

AWeaponBaseServer* AFPSBaseCharacter::GetCurrentServerTPBodysWeaponAtcor() {
	switch(ActiveWeapon) {
		case EWeaponType::AK47: {
				return ServerPrimaryWeapon;
			}
		case EWeaponType::M4A1: {
				return ServerPrimaryWeapon;
			}
		case EWeaponType::MP7: {
				return ServerPrimaryWeapon;
			}
		case EWeaponType::DesertEagle: {
				return ServerSecondaryWeapon;
			}
		case EWeaponType::Sniper: {
				return ServerPrimaryWeapon;
			}
	}
	return nullptr;
}
#pragma endregion 

/* 换弹与射击相关 */
#pragma region Fire
void AFPSBaseCharacter::AutomaticFire() {
	// 判断弹匣子弹是否足够
	if(ServerPrimaryWeapon->ClipCurrentAmmo>0) {
		if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f) {
			ServerFireRifleWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		} else {
			ServerFireRifleWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);	// 先传一个不移动(后面判断是否移动)
		}
		ClientFire();
		ClientRecoil();
	} else {
		// 子弹卡壳声效
		StopFirePrimary();
	}
}

void AFPSBaseCharacter::ResetRecoil() {
	NewVerticalRecoilAmount = 0;
	OldVerticalRecoilAmount = 0;
	VerticalRecoilAmount = 0;
	RecoilXCoordPerShoot = 0;
}

void AFPSBaseCharacter::FireWeaponPrimary() {
	// 判断弹匣子弹是否足够 | 换弹前强制射击去掉 !IsReloading
	if(ServerPrimaryWeapon->ClipCurrentAmmo>0 && !IsReloading) {
		// 服务端：减少弹药 | 射线检测 (三种) | 伤害应用 | 弹孔生成 枪口特效 射击声效
		if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f) {
			ServerFireRifleWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		} else {
			ServerFireRifleWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);	// 先传一个不移动(后面判断是否移动)
		}
		// 客户端：枪体动画 | 手臂动画 | 射击声效 | 屏幕抖动 | 后作力 | 枪口特效
		ClientFire();
		ClientRecoil();
		//UKismetSystemLibrary::PrintString(this,FString::Printf(TEXT("ServerPrimaryWeapon->ClipCurrentAmmo: %d"),ServerPrimaryWeapon->ClipCurrentAmmo));  // DeBug输出子弹数

		// 射击模式：连发(开启计时器) | 单射 | 点发
		if(ServerPrimaryWeapon->IsAutomatic) {
			GetWorldTimerManager().SetTimer(AutomaticFireTimerHandle,this,&AFPSBaseCharacter::AutomaticFire,ServerPrimaryWeapon->AutomaticFireRate,true);
		}
	} else {
		// 子弹卡壳声效
	}
}
void AFPSBaseCharacter::StopFirePrimary() {
	// 析构FireWeaponPrimary 参数 关闭计时器
	ServerStopFiring();
	GetWorldTimerManager().ClearTimer(AutomaticFireTimerHandle);
	ResetRecoil();
}
/* 步枪射击射线检测 */
void AFPSBaseCharacter::RifleLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving) {
	FVector EndLocation;
	FVector CameraForwardVector =  UKismetMathLibrary::GetForwardVector(CameraRotation);	// 前向向量
	TArray<AActor*> IgnoreArray;	// 要被忽略的碰撞检测数组
	IgnoreArray.Add(this);
	FHitResult HitResult;	// 接收射线检测后结果

	if(ServerPrimaryWeapon) {
		/* EndLocation计算方法 IsMoving 是否移动会导致不同的EndLocation计算 */
		if(IsMoving) {
			FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulletDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			EndLocation = FVector(Vector.X + RandomX,Vector.Y + RandomY,Vector.Z + RandomZ);
		}else {
			EndLocation = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulletDistance;
		}
	}
	/**
	 *  第五个为 false 目前为简单的碰撞  true为复杂碰撞
	 *  第七个为是否画出射击检测的DeBug线 Persistent 永久画出测试使用 改为 None则关闭
	 *  第十个为DeBug 线颜色 FLinearColor::Red
	 *  第十一个为击中后颜色 FLinearColor::Green
	 *  最后一个是DeBug线存在时间，目前为永久
	 */
	bool HitSuccess = UKismetSystemLibrary::LineTraceSingle(GetWorld(),CameraLocation,EndLocation,ETraceTypeQuery::TraceTypeQuery1,false,IgnoreArray,EDrawDebugTrace::None,HitResult,true,FLinearColor::Red,FLinearColor::Green,3.f);	

	/* 如果射线检测成功了去实现方法：打到玩家应用伤害 打到墙生成弹孔 打到可破坏墙就破碎 */
	if(HitSuccess) {
		//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hit Actor Name : %s"),*HitResult.GetActor()->GetName()));	// 射线检测日志
		AFPSBaseCharacter* FPSCharacter = Cast<AFPSBaseCharacter>(HitResult.GetActor());		// 接收检测玩家
		if(FPSCharacter) {
			// 打到玩家应用伤害
			DamagePlayer(HitResult.PhysMaterial.Get(),HitResult.GetActor(),CameraLocation,HitResult);
		} else {
			// 打到非玩家广播弹孔
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);	// 让弹孔Rotation与模型法线方向一致
			MultiSpawnBulletDecal(HitResult.Location,XRotator);
		}
	}
}

/* 狙击枪相关射击方法 */
void AFPSBaseCharacter::FireWeaponSniper() {
	// 判断弹匣子弹是否足够 | 换弹前强制射击去掉 !IsReloading
	if(ServerPrimaryWeapon->ClipCurrentAmmo>0 && !IsReloading && !IsFiring) {
		// 服务端：减少弹药 | 射线检测 (三种) | 伤害应用 | 弹孔生成 枪口特效 射击声效
		if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f) {
			ServerFireSniperWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		} else {
			ServerFireSniperWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);	// 先传一个不移动(后面判断是否移动)
		}
		ClientFire();
	} else {
		// 子弹卡壳声效
	}
}
void AFPSBaseCharacter::StopFireSniper() {
}
void AFPSBaseCharacter::SniperLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving) {
	FVector EndLocation;
	FVector CameraForwardVector =  UKismetMathLibrary::GetForwardVector(CameraRotation);	// 前向向量
	TArray<AActor*> IgnoreArray;	// 要被忽略的碰撞检测数组
	IgnoreArray.Add(this);
	FHitResult HitResult;	// 接收射线检测后结果

	if(ServerPrimaryWeapon) {
		/* 是否进行瞄准 IsAiming 是否控制开镜 */
		if(IsAiming) {
			/* EndLocation计算方法 IsMoving 是否移动会导致不同的EndLocation计算 */
			if(IsMoving) {
				FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulletDistance;
				float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
				float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
				float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
				EndLocation = FVector(Vector.X + RandomX,Vector.Y + RandomY,Vector.Z + RandomZ);
			}else {
				EndLocation = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulletDistance;
			}
			ClientEndAiming();	// 射击结束后自动关镜
		}else {
			FVector Vector = CameraLocation + CameraForwardVector * ServerPrimaryWeapon->BulletDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			EndLocation = FVector(Vector.X + RandomX,Vector.Y + RandomY,Vector.Z + RandomZ);
		}
		
	}
	/**
	 *  第五个为 false 目前为简单的碰撞  true为复杂碰撞
	 *  第七个为是否画出射击检测的DeBug线 Persistent 永久画出测试使用 改为 None则关闭
	 *  第十个为DeBug 线颜色 FLinearColor::Red
	 *  第十一个为击中后颜色 FLinearColor::Green
	 *  最后一个是DeBug线存在时间，目前为永久
	 */
	bool HitSuccess = UKismetSystemLibrary::LineTraceSingle(GetWorld(),CameraLocation,EndLocation,ETraceTypeQuery::TraceTypeQuery1,false,IgnoreArray,EDrawDebugTrace::None,HitResult,true,FLinearColor::Red,FLinearColor::Green,3.f);	

	/* 如果射线检测成功了去实现方法：打到玩家应用伤害 打到墙生成弹孔 打到可破坏墙就破碎 */
	if(HitSuccess) {
		//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hit Actor Name : %s"),*HitResult.GetActor()->GetName()));	// 射线检测日志
		AFPSBaseCharacter* FPSCharacter = Cast<AFPSBaseCharacter>(HitResult.GetActor());		// 接收检测玩家
		if(FPSCharacter) {
			// 打到玩家应用伤害
			DamagePlayer(HitResult.PhysMaterial.Get(),HitResult.GetActor(),CameraLocation,HitResult);
		} else {
			// 打到非玩家广播弹孔
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);	// 让弹孔Rotation与模型法线方向一致
			MultiSpawnBulletDecal(HitResult.Location,XRotator);
		}
	}
}

/* 手枪相关射击方法 */
void AFPSBaseCharacter::FireWeaponSecondary() {
	// 判断弹匣子弹是否足够 | 换弹前强制射击去掉 !IsReloading
	if(ServerSecondaryWeapon->ClipCurrentAmmo>0 && !IsReloading) {
		// 服务端：减少弹药 | 射线检测 (三种) | 伤害应用 | 弹孔生成 枪口特效 射击声效
		if(UKismetMathLibrary::VSize(GetVelocity()) > 0.1f) {
			ServerFirePistolWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		} else {
			ServerFirePistolWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);	// 先传一个不移动(后面判断是否移动)
		}
		ClientFire();
	} else {
		// 子弹卡壳声效
	}
}
void AFPSBaseCharacter::StopFireSecondary() {
	// 更改IsFiring变量
	ServerStopFiring();
}

/* 射线检测 */
void AFPSBaseCharacter::PistolLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving){
	FVector EndLocation;
	FVector CameraForwardVector;	// 前向向量
	TArray<AActor*> IgnoreArray;	// 要被忽略的碰撞检测数组
	IgnoreArray.Add(this);
	FHitResult HitResult;	// 接收射线检测后结果

	if(ServerSecondaryWeapon) {
		/* EndLocation计算方法 IsMoving 是否移动会导致不同的EndLocation计算 */
		if(IsMoving) {
			FRotator  Rotator;
			Rotator.Roll = CameraRotation.Roll;
			Rotator.Pitch = CameraRotation.Pitch + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin,PistolSpreadMax);
			Rotator.Yaw = CameraRotation.Yaw + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin,PistolSpreadMax);
			CameraForwardVector =  UKismetMathLibrary::GetForwardVector(Rotator);	// 前向向量
			FVector Vector = CameraLocation + CameraForwardVector * ServerSecondaryWeapon->BulletDistance;
			float RandomX = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon->MovingFireRandomRange,ServerSecondaryWeapon->MovingFireRandomRange);
			float RandomY = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon->MovingFireRandomRange,ServerSecondaryWeapon->MovingFireRandomRange);
			float RandomZ = UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon->MovingFireRandomRange,ServerSecondaryWeapon->MovingFireRandomRange);
			EndLocation = FVector(Vector.X + RandomX,Vector.Y + RandomY,Vector.Z + RandomZ);
		}else {
			// 旋转随机偏移 根据连续射击快慢决定 连续越快偏移越大
			FRotator  Rotator;
			Rotator.Roll = CameraRotation.Roll;
			Rotator.Pitch = CameraRotation.Pitch + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin,PistolSpreadMax);
			Rotator.Yaw = CameraRotation.Yaw + UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin,PistolSpreadMax);
			CameraForwardVector =  UKismetMathLibrary::GetForwardVector(Rotator);	// 前向向量
			EndLocation = CameraLocation + CameraForwardVector * ServerSecondaryWeapon->BulletDistance;
		}
	}
	/**
	 *  第五个为 false 目前为简单的碰撞  true为复杂碰撞
	 *  第七个为是否画出射击检测的DeBug线 Persistent 永久画出测试使用 改为 None则关闭
	 *  第十个为DeBug 线颜色 FLinearColor::Red
	 *  第十一个为击中后颜色 FLinearColor::Green
	 *  最后一个是DeBug线存在时间，目前为永久
	 */
	bool HitSuccess = UKismetSystemLibrary::LineTraceSingle(GetWorld(),CameraLocation,EndLocation,ETraceTypeQuery::TraceTypeQuery1,false,IgnoreArray,EDrawDebugTrace::None,HitResult,true,FLinearColor::Red,FLinearColor::Green,3.f);

	PistolSpreadMax += ServerSecondaryWeapon->SpreadWeaponMaxIndex;
	PistolSpreadMin -= ServerSecondaryWeapon->SpreadWeaponMinIndex;

	/* 如果射线检测成功了去实现方法：打到玩家应用伤害 打到墙生成弹孔 打到可破坏墙就破碎 */
	if(HitSuccess) {
		//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hit Actor Name : %s"),*HitResult.GetActor()->GetName()));	// 射线检测日志
		AFPSBaseCharacter* FPSCharacter = Cast<AFPSBaseCharacter>(HitResult.GetActor());		// 接收检测玩家
		if(FPSCharacter) {
			// 打到玩家应用伤害
			DamagePlayer(HitResult.PhysMaterial.Get(),HitResult.GetActor(),CameraLocation,HitResult);
		} else {
			// 打到非玩家广播弹孔
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);	// 让弹孔Rotation与模型法线方向一致
			MultiSpawnBulletDecal(HitResult.Location,XRotator);
		}
	}
}

/* 步枪换弹动画后的回调 */
void AFPSBaseCharacter::DelayPlayArmReloadCallBack() {
	AWeaponBaseServer* CurrentServerWeapon = GetCurrentServerTPBodysWeaponAtcor();
	if (CurrentServerWeapon) {
		int32 GunCurrentAmmo = CurrentServerWeapon->GunCurrentAmmo;
		int32 ClipCurrentAmmo = CurrentServerWeapon->ClipCurrentAmmo;
		int32 const MaxClipAmmo = CurrentServerWeapon->MaxClipAmmo;
		IsReloading = false;
		// 是否装填全部枪体子弹
		if(MaxClipAmmo - ClipCurrentAmmo >= GunCurrentAmmo) {
			ClipCurrentAmmo += GunCurrentAmmo;
			GunCurrentAmmo = 0;
		} else {
			GunCurrentAmmo -= MaxClipAmmo - ClipCurrentAmmo;
			ClipCurrentAmmo = MaxClipAmmo;
		}
		CurrentServerWeapon->GunCurrentAmmo = GunCurrentAmmo;
		CurrentServerWeapon->ClipCurrentAmmo = ClipCurrentAmmo;
		ClientUpdateAmmoUI(ClipCurrentAmmo,GunCurrentAmmo);
		//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("DelayPlayArmReloadCallBack()")));
	}
}
/* 手枪换弹动画后的回调 */
void AFPSBaseCharacter::DelaySpreadWeaponShootCallBack() {
	PistolSpreadMin = 0;
	PistolSpreadMax = 0;
}
/* 狙击枪换弹动画后的回调 */
void AFPSBaseCharacter::DelaySniperShootCallBack() {
	IsFiring = false;
	
}

void AFPSBaseCharacter::DamagePlayer(UPhysicalMaterial* PhysicalMaterial,AActor* DamagedActor,FVector& HitFromDirection,FHitResult& HitInfo) {
	/* 玩家伤害不同部位 */
	if(ServerPrimaryWeapon) {
		switch (PhysicalMaterial->SurfaceType) {
			case EPhysicalSurface::SurfaceType1: {
					// Head
					UGameplayStatics::ApplyPointDamage(DamagedActor,ServerPrimaryWeapon->BaseDamage * 4,HitFromDirection,HitInfo,GetController(),this,UDamageType::StaticClass());
			}
				break;
			case EPhysicalSurface::SurfaceType2: {
					// Body
					UGameplayStatics::ApplyPointDamage(DamagedActor,ServerPrimaryWeapon->BaseDamage * 1,HitFromDirection,HitInfo,GetController(),this,UDamageType::StaticClass());
			}
				break;
			case EPhysicalSurface::SurfaceType3: {
					// Arm
					UGameplayStatics::ApplyPointDamage(DamagedActor,ServerPrimaryWeapon->BaseDamage * 0.8,HitFromDirection,HitInfo,GetController(),this,UDamageType::StaticClass());
			}
				break;
			case EPhysicalSurface::SurfaceType4: {
					// Leg
					UGameplayStatics::ApplyPointDamage(DamagedActor,ServerPrimaryWeapon->BaseDamage * 0.7,HitFromDirection,HitInfo,GetController(),this,UDamageType::StaticClass());
			}
				break;
		}
	}
}
void AFPSBaseCharacter::OnHit(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType,AActor* DamageCauser) {
	Health -= Damage;
	ClientUpdateHealthUI(Health);
	if(Health <= 0 ) {
		// 死亡逻辑
	}
	//UKismetSystemLibrary::PrintString(this,FString::Printf(TEXT("PlayerName%s Health : %f"),*GetName(),Health));	// 伤害调试日志
}
#pragma endregion
