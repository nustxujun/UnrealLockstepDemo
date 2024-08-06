#pragma once 

#include "CoreMinimal.h"
#include "ArxRenderable.h"
#include "GameFramework/Character.h"

#include "RenderBall.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ARenderBall : public ACharacter, public IArxRenderable
{
    GENERATED_UCLASS_BODY()

		/** Camera boom positioning the camera behind the character */
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;


	UPROPERTY()
	class UArxSmoothMoveComponent* SmoothComponent;
public: 
    virtual void LinkEntity(ArxEntity* Entity) override;
    virtual void UnlinkEntity() override;
	virtual void OnFrame(int FrameId) override;


	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
    Rp3dTransform CachedTrans;
	Rp3dVector3 MoveForwardValue = {};
	Rp3dVector3 MoveRightValue = {};
};