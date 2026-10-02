#include "General/CameraPawn.h"

#include "DataSubsystem.h"
#include "General/FunctionLibrary.h"
#include "Interaction/UnitSceneSubsystem.h"
#include "UI/ConfiguratorWidget.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

ACameraPawn::ACameraPawn() {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.bStartWithTickEnabled = false;
}

void ACameraPawn::BeginPlay() {
  Super::BeginPlay();

  SpringArm = FindComponentByClass<USpringArmComponent>();
  const UCameraComponent* Camera = FindComponentByClass<UCameraComponent>();
  check(SpringArm);
  check(Camera);

  GenplanDistance = SpringArm->TargetArmLength;
  GenplanRotation = SpringArm->GetRelativeRotation();

  CurrentView = CaptureView();
  if (UGameInstance* GameInstance = GetGameInstance()) {
    if (UDataSubsystem* Data = GameInstance->GetSubsystem<UDataSubsystem>()) {
      Data->OnLoadFinished.AddDynamic(this, &ACameraPawn::HandleDataLoaded);
      if (Data->HasData()) HandleDataLoaded(true, TEXT(""));
    }
  }
}

void ACameraPawn::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  if (ConfiguratorWidget) {
    ConfiguratorWidget->RemoveFromParent();
    ConfiguratorWidget = nullptr;
  }

  if (UGameInstance* GameInstance = GetGameInstance()) {
    if (UDataSubsystem* Data = GameInstance->GetSubsystem<UDataSubsystem>()) {
      Data->OnLoadFinished.RemoveDynamic(this, &ACameraPawn::HandleDataLoaded);
    }
  }
  Super::EndPlay(EndPlayReason);
}

void ACameraPawn::PossessedBy(AController* NewController) {
  Super::PossessedBy(NewController);
  if (APlayerController* Player = Cast<APlayerController>(NewController)) {
    UGeneralFunctionLibrary::SetInputMode(Player, EViewportInputMode::GameAndUI);
    Player->bEnableClickEvents = true;

    if (ConfiguratorWidgetClass && !ConfiguratorWidget) {
      ConfiguratorWidget = CreateWidget<UConfiguratorWidget>(Player, ConfiguratorWidgetClass);
      if (ConfiguratorWidget) {
        ConfiguratorWidget->SetCameraPawn(this);
        ConfiguratorWidget->AddToViewport();
      }
    }
  }
}

void ACameraPawn::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  if (bTransitioning) {
    TransitionTime += DeltaTime;
    const float Alpha = FMath::Clamp(TransitionTime / TransitionDuration, 0.0f, 1.0f);
    const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

    SetActorLocation(FMath::Lerp(TransitionStart.FocusPoint, CurrentView.FocusPoint, SmoothAlpha));
    SpringArm->TargetArmLength = FMath::Lerp(TransitionStart.Distance, CurrentView.Distance, SmoothAlpha);
    SpringArm->SetRelativeRotation(FQuat::Slerp(TransitionStart.Rotation.Quaternion(), CurrentView.Rotation.Quaternion(), SmoothAlpha).Rotator());

    if (Alpha >= 1.0f) bTransitioning = false;
  }

  if (!bTransitioning) SetActorTickEnabled(false);
}

bool ACameraPawn::ShowGenplan() {
  if (!SpringArm) return false;
  UGameInstance* GameInstance = GetGameInstance();
  const UDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDataSubsystem>() : nullptr;
  if (!Data || !Data->HasData()) return false;

  FView View;
  View.FocusPoint = Data->GetBuildingData().GenplanFocusPoint;
  View.Rotation = GenplanRotation;
  View.Distance = GenplanDistance;
  View.Mode = ECameraViewMode::Genplan;
  StartTransition(View, true);
  return true;
}

bool ACameraPawn::ShowFloor(int32 FloorNumber) {
  if (!SpringArm || FloorDistance <= 0.0f) return false;
  UGameInstance* GameInstance = GetGameInstance();
  const UDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDataSubsystem>() : nullptr;
  if (!Data || !Data->HasData()) return false;

  for (const FFloorData& Floor : Data->GetBuildingData().Floors) {
    if (Floor.Number != FloorNumber) continue;

    FView View;
    View.FocusPoint = Floor.FocusPoint;
    View.Rotation = CurrentView.Rotation;
    View.Distance = FloorDistance;
    View.Mode = ECameraViewMode::Floor;
    View.SelectedId = Floor.Id;
    StartTransition(View, true);
    return true;
  }
  return false;
}

bool ACameraPawn::ShowUnit(const FString& UnitId) {
  if (!SpringArm || UnitDistance <= 0.0f) return false;
  UGameInstance* GameInstance = GetGameInstance();
  const UDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDataSubsystem>() : nullptr;
  if (!Data || !Data->HasData()) return false;

  for (const FFloorData& Floor : Data->GetBuildingData().Floors) {
    for (const FUnitData& Unit : Floor.Apartments) {
      if (Unit.Id != UnitId) continue;

      FView View;
      View.FocusPoint = Unit.FocusPoint;
      View.Rotation = CurrentView.Rotation;
      View.Distance = UnitDistance;
      View.Mode = ECameraViewMode::Unit;
      View.SelectedId = Unit.Id;
      StartTransition(View, true);
      return true;
    }
  }
  return false;
}

bool ACameraPawn::GoBack() {
  if (!SpringArm || History.IsEmpty()) return false;
  const FView PreviousView = History.Pop();
  StartTransition(PreviousView, false);
  return true;
}

void ACameraPawn::Orbit(float HorizontalDelta, float VerticalDelta) {
  if (!SpringArm || CurrentView.Mode != ECameraViewMode::Genplan || bTransitioning
    || OrbitSensitivity <= 0.0f || MinOrbitPitch >= MaxOrbitPitch) return;

  FRotator Rotation = SpringArm->GetRelativeRotation();
  Rotation.Yaw += HorizontalDelta * OrbitSensitivity;
  Rotation.Pitch = FMath::Clamp(Rotation.Pitch - VerticalDelta * OrbitSensitivity, MinOrbitPitch, MaxOrbitPitch);
  SpringArm->SetRelativeRotation(Rotation);
  CurrentView.Rotation = Rotation;
}

void ACameraPawn::HandleDataLoaded(bool bSuccess, FString Message) {
  if (!bSuccess) return;

  History.Reset();
  bHasView = false;
  ShowGenplan();
}

ACameraPawn::FView ACameraPawn::CaptureView() const {
  FView View;
  View.FocusPoint = GetActorLocation();
  View.Rotation = SpringArm->GetRelativeRotation();
  View.Distance = SpringArm->TargetArmLength;
  return View;
}

void ACameraPawn::StartTransition(const FView& NewView, bool bSaveCurrent) {
  if (bSaveCurrent && bHasView
    && (CurrentView.Mode != NewView.Mode || CurrentView.SelectedId != NewView.SelectedId)) {
    History.Add(CurrentView);
  }

  TransitionStart = CaptureView();
  CurrentView = NewView;
  bHasView = true;
  TransitionTime = 0.0f;
  bTransitioning = true;
  SetActorTickEnabled(true);
  if (UUnitSceneSubsystem* Scene = GetWorld()->GetSubsystem<UUnitSceneSubsystem>()) {
    Scene->SetSelectedUnit(NewView.Mode == ECameraViewMode::Unit ? NewView.SelectedId : FString());
  }
  OnViewChanged.Broadcast(CurrentView.Mode, CurrentView.SelectedId);
}
