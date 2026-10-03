#include "Interaction/UnitComponent.h"

#include "DataSubsystem.h"
#include "DataTypes.h"
#include "General/CameraPawn.h"
#include "General/FunctionLibrary.h"
#include "Interaction/UnitSceneSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInterface.h"

void UUnitComponent::BeginPlay() {
  Super::BeginPlay();

  UnitId.TrimStartAndEndInline();
  // Запоминаем материалы выбранных мешей, чтобы вернуть их после снятия эффекта
  for (const FComponentReference& Reference : VisualMeshes) {
    UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Reference.GetComponent(GetOwner()));
    if (!Mesh || Mesh->GetOwner() != GetOwner() || ResolvedMeshes.Contains(Mesh)) continue;

    ResolvedMeshes.Add(Mesh);
    OriginalMaterials.Add(Mesh->GetNumMaterials() > 0 ? Mesh->GetMaterial(0) : nullptr);
  }

  GetOwner()->OnClicked.AddUniqueDynamic(this, &UUnitComponent::HandleActorClicked);

  if (UUnitSceneSubsystem* Scene = GetWorld()->GetSubsystem<UUnitSceneSubsystem>()) {
    Scene->AddUnit(this);
  }

  if (UGameInstance* GameInstance = GetWorld()->GetGameInstance()) {
    DataSubsystem = GameInstance->GetSubsystem<UDataSubsystem>();
  }
  if (DataSubsystem) {
    DataSubsystem->OnLoadFinished.AddUniqueDynamic(this, &UUnitComponent::HandleDataLoaded);
    // Данные могли загрузиться раньше, чем появился этот Actor
    if (DataSubsystem->HasData()) ResolveStatus();
  }

  if (UnitId.IsEmpty()) {
    UGeneralFunctionLibrary::PrintLog(TEXT("UnitComponent has no UnitId."), EPrintLogLevel::Warning);
  }
}

void UUnitComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  if (DataSubsystem) {
    DataSubsystem->OnLoadFinished.RemoveDynamic(this, &UUnitComponent::HandleDataLoaded);
  }
  GetOwner()->OnClicked.RemoveDynamic(this, &UUnitComponent::HandleActorClicked);
  if (UWorld* World = GetWorld()) {
    if (UUnitSceneSubsystem* Scene = World->GetSubsystem<UUnitSceneSubsystem>()) {
      Scene->RemoveUnit(this);
    }
  }
  Super::EndPlay(EndPlayReason);
}

void UUnitComponent::ResolveStatus() {
  bHasStatus = false;
  if (!DataSubsystem || !DataSubsystem->HasData() || UnitId.IsEmpty()) return;

  for (const FFloorData& Floor : DataSubsystem->GetBuildingData().Floors) {
    for (const FUnitData& Unit : Floor.Apartments) {
      if (Unit.Id != UnitId) continue;
      bHasStatus = true;
      bIsSold = Unit.Status == EUnitStatus::Sold;
      break;
    }
    if (bHasStatus) break;
  }

  if (!bHasStatus) {
    UGeneralFunctionLibrary::PrintLog(
      FString::Printf(TEXT("Unit ID '%s' is missing from JSON."), *UnitId), EPrintLogLevel::Warning);
  }

  if (UUnitSceneSubsystem* Scene = GetWorld()->GetSubsystem<UUnitSceneSubsystem>()) {
    Scene->RefreshUnit(this);
  }
}

// применение визуальный эффектов если sold или selected
void UUnitComponent::ApplyVisuals(bool bSelected, bool bHideSold) {
  for (int32 Index = 0; Index < ResolvedMeshes.Num(); ++Index) {
    UStaticMeshComponent* Mesh = ResolvedMeshes[Index];
    if (!Mesh) continue;

    if (Mesh->GetNumMaterials() == 0) continue;
    UMaterialInterface* Material = OriginalMaterials[Index];
    if (bSelected && SelectedMaterial) {
      Material = SelectedMaterial;
    } else if (bHideSold && bHasStatus && bIsSold && SoldMaterial) {
      Material = SoldMaterial;
    }
    if (Mesh->GetMaterial(0) != Material) Mesh->SetMaterial(0, Material);
  }
}

void UUnitComponent::HandleDataLoaded(bool bSuccess, FString Message) {
  if (bSuccess) ResolveStatus();
}

void UUnitComponent::HandleActorClicked(AActor* TouchedActor, FKey ButtonPressed) {
  if (ButtonPressed != EKeys::LeftMouseButton || UnitId.IsEmpty()) return;

  APlayerController* Player = GetWorld()->GetFirstPlayerController();
  ACameraPawn* CameraPawn = Player ? Cast<ACameraPawn>(Player->GetPawn()) : nullptr;
  if (CameraPawn) CameraPawn->ShowUnit(UnitId);
}
