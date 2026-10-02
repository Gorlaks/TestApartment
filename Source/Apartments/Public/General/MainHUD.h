// Создаёт интерфейс конфигуратора и подключает его к текущей камере
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

class UConfiguratorWidget;
class APawn;

UCLASS()
class APARTMENTS_API AMainHUD : public AHUD {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintPure)
  UConfiguratorWidget* GetConfiguratorWidget() const { return ConfiguratorWidget; }

protected:
  virtual void BeginPlay() override;

private:
  UPROPERTY(EditDefaultsOnly)
  TSubclassOf<UConfiguratorWidget> ConfiguratorWidgetClass;

  UPROPERTY(Transient)
  TObjectPtr<UConfiguratorWidget> ConfiguratorWidget;

  UFUNCTION()
  void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
};
