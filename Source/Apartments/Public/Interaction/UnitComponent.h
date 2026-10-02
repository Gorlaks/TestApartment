// Связывает объект квартиры в сцене с данными и обработкой клика
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitComponent.generated.h"

class UDataSubsystem;
class UMaterialInterface;
class UMeshComponent;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class APARTMENTS_API UUnitComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintPure)
  FString GetUnitId() const { return UnitId; }
  void ApplyVisuals(bool bSelected, bool bHideSold);

protected:
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
  // UnitId задаётся у экземпляра и совпадает с ID в json
  UPROPERTY(EditInstanceOnly)
  FString UnitId;

  UPROPERTY(EditDefaultsOnly)
  TObjectPtr<UMaterialInterface> SelectedMaterial;

  UPROPERTY(EditDefaultsOnly)
  TObjectPtr<UMaterialInterface> SoldMaterial;

  UPROPERTY(Transient)
  TObjectPtr<UDataSubsystem> DataSubsystem;

  UPROPERTY(Transient)
  TObjectPtr<UMeshComponent> VisualMesh;

  UPROPERTY(Transient)
  TObjectPtr<UMaterialInterface> OriginalMaterial;

  bool bHasStatus = false;
  bool bIsSold = false;

  void ResolveStatus();

  UFUNCTION()
  void HandleDataLoaded(bool bSuccess, FString Message);

  UFUNCTION()
  void HandleActorClicked(AActor* TouchedActor, FKey ButtonPressed);
};
