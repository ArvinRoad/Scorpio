#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBaseClien.generated.h"

UCLASS()
class SCORPIO_API AWeaponBaseClien : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBaseClien();

	UPROPERTY(EditAnywhere)
	USkeletalMeshComponent* WeaponMesh;
	

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
