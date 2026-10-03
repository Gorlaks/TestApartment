// Связывает объект квартиры в сцене с данными и обработкой клика
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "UnitComponent.generated.h"

class UDataSubsystem;
class UMaterialInterface;
class UStaticMeshComponent;

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

  // Меши, на которых меняется материал при выборе квартиры и включении фильтра
  UPROPERTY(EditDefaultsOnly, meta = (UseComponentPicker, AllowedClasses = "/Script/Engine.StaticMeshComponent"))
  TArray<FComponentReference> VisualMeshes;

  UPROPERTY(Transient)
  TObjectPtr<UDataSubsystem> DataSubsystem;

  UPROPERTY(Transient)
  TArray<TObjectPtr<UStaticMeshComponent>> ResolvedMeshes;

  UPROPERTY(Transient)
  TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

  bool bHasStatus = false;
  bool bIsSold = false;

  void ResolveStatus();

  UFUNCTION()
  void HandleDataLoaded(bool bSuccess, FString Message);

  UFUNCTION()
  void HandleActorClicked(AActor* TouchedActor, FKey ButtonPressed);
};
