#include "General/MainHUD.h"

#include "General/CameraPawn.h"
#include "General/FunctionLibrary.h"
#include "UI/ConfiguratorWidget.h"
#include "GameFramework/PlayerController.h"

void AMainHUD::BeginPlay() {
  Super::BeginPlay();

  APlayerController* Player = GetOwningPlayerController();
  if (!Player || !Player->IsLocalController()) return;

  UGeneralFunctionLibrary::SetInputMode(Player, EViewportInputMode::GameAndUI);
  Player->bEnableClickEvents = true;

  if (!ConfiguratorWidgetClass) return;
  ConfiguratorWidget = CreateWidget<UConfiguratorWidget>(Player, ConfiguratorWidgetClass);
  if (!ConfiguratorWidget) return;

  ConfiguratorWidget->AddToViewport();
  ConfiguratorWidget->SetCameraPawn(Cast<ACameraPawn>(Player->GetPawn()));
  // Pawn может появиться позже HUD, поэтому обновляем ссылку при его смене
  Player->OnPossessedPawnChanged.AddUniqueDynamic(this, &AMainHUD::HandlePawnChanged);
}

void AMainHUD::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn) {
  if (ConfiguratorWidget) ConfiguratorWidget->SetCameraPawn(Cast<ACameraPawn>(NewPawn));
}
