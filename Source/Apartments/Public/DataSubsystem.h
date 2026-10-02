#pragma once

#include "CoreMinimal.h"
#include "DataTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DataSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDataLoadFinished, bool, bSuccess, FString, Message);

UCLASS()
class APARTMENTS_API UDataSubsystem : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable)
  bool LoadDefaultData();

  UFUNCTION(BlueprintCallable)
  bool LoadFromFile(const FString& FilePath);

  UFUNCTION(BlueprintPure)
  bool IsLoading() const { return bIsLoading; }

  UFUNCTION(BlueprintPure)
  bool HasData() const { return bHasData; }

  UFUNCTION(BlueprintPure)
  FBuildingData GetBuildingData() const;

  UPROPERTY(BlueprintAssignable)
  FOnDataLoadFinished OnLoadFinished;

  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;

private:
  UPROPERTY()
  FBuildingData BuildingData;

  bool bIsLoading = false;
  bool bHasData = false;
};
