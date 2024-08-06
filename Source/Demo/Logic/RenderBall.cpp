#include "RenderBall.h"

#include "ArxWorld.h"
#include "ArxSmoothMoveComponent.h"

#include "Ball.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

ARenderBall::ARenderBall(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{


	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm


	SmoothComponent= CreateDefaultSubobject<UArxSmoothMoveComponent>(TEXT("SmoothMoveComponent"));
}

void ARenderBall::LinkEntity(ArxEntity* Ent)
{
	IArxRenderable::LinkEntity(Ent);
}

void ARenderBall::UnlinkEntity()
{
	IArxRenderable::UnlinkEntity();

}

void ARenderBall::OnFrame(int FrameId)
{
	auto Ent = GetEntity();
	if (!Ent)
		return;
	check(Ent);
	auto Char = static_cast<Ball*>(Ent);

	auto& Trans = Char->GetTransform();

	SmoothComponent->OnFrame(FrameId, RP3D_TO_UE(Trans));
}




void ARenderBall::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ARenderBall::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ARenderBall::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ARenderBall::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ARenderBall::LookUpAtRate);


}




void ARenderBall::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ARenderBall::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ARenderBall::MoveForward(float Value)
{
	if ((Controller != nullptr) )
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		//AddMovementInput(Direction, Value);

		auto Fwd = UE_TO_RP3D(Direction * Value );
		if (MoveForwardValue != Fwd)
		{
			MoveForwardValue = Fwd;
			auto Ent = static_cast<Ball*>(GetEntity());
			Ent->Move(MoveForwardValue);
		}
	}
}

void ARenderBall::MoveRight(float Value)
{
	if ((Controller != nullptr))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		//AddMovementInput(Direction, Value);

		auto Rgt = UE_TO_RP3D(Direction * Value );
		if (MoveRightValue != Rgt)
		{
			MoveRightValue = Rgt;
			auto Ent = static_cast<Ball*>(GetEntity());
			Ent->Move(Rgt);
		}
	}
}