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
		
	}
}
#pragma endregion 
