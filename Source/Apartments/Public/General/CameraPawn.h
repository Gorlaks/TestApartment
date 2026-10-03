// Камера и переходы между общим видом, этажом и квартирой
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CameraPawn.generated.h"

class USpringArmComponent;

UENUM(BlueprintType)
enum class ECameraViewMode : uint8 {
  Genplan,
  Floor,
  Unit
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCameraViewChanged, ECameraViewMode, Mode, FString, SelectedId);

UCLASS()
class APARTMENTS_API ACameraPawn : public APawn {
  GENERATED_BODY()

public:
  ACameraPawn();

  UFUNCTION(BlueprintCallable)
  bool ShowGenplan();

  UFUNCTION(BlueprintCallable)
  bool ShowFloor(int32 FloorNumber);

  UFUNCTION(BlueprintCallable)
  bool ShowUnit(const FString& UnitId);

  UFUNCTION(BlueprintCallable)
  bool GoBack();

  UFUNCTION(BlueprintCallable)
  void Orbit(float HorizontalDelta, float VerticalDelta);

  UFUNCTION(BlueprintPure)
  bool CanGoBack() const { return !History.IsEmpty(); }

  UFUNCTION(BlueprintPure)
  ECameraViewMode GetViewMode() const { return CurrentView.Mode; }

  UFUNCTION(BlueprintPure)
  FString GetSelectedId() const { return CurrentView.SelectedId; }

  UPROPERTY(BlueprintAssignable)
  FOnCameraViewChanged OnViewChanged;

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;

private:
  // Ракурс камеры (то же самое сохраняется и в History)
  struct FView {
    FVector FocusPoint = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    float Distance = 0.0f;
    ECameraViewMode Mode = ECameraViewMode::Genplan;
    FString SelectedId;
  };

  UPROPERTY(Transient)
  TObjectPtr<USpringArmComponent> SpringArm;

  UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.01"))
  float TransitionDuration;

  UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1.0"))
  float FloorDistance;

  UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1.0"))
  float UnitDistance;

  UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.001"))
  float OrbitSensitivity;

  UPROPERTY(EditDefaultsOnly)
  float MinOrbitPitch;

  UPROPERTY(EditDefaultsOnly)
  float MaxOrbitPitch;

  FView CurrentView;
  FView TransitionStart;
  TArray<FView> History;
  FRotator GenplanRotation = FRotator::ZeroRotator;
  float GenplanDistance = 0.0f;
  float TransitionTime = 0.0f;
  bool bHasView = false;
  bool bTransitioning = false;

  UFUNCTION()
  void HandleDataLoaded(bool bSuccess, FString Message);

  FView CaptureView() const;
  void StartTransition(const FView& NewView, bool bSaveCurrent);
};
