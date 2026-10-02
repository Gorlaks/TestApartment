#include "FunctionLibrary.h"

#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogGeneral, Log, All);
//TODO
void UGeneralFunctionLibrary::PrintLog(const FString& Message, EPrintLogLevel Level) {
  switch (Level) {
  case EPrintLogLevel::Warning:
    UE_LOG(LogGeneral, Warning, TEXT("%s"), *Message);
    break;

  case EPrintLogLevel::Error:
    UE_LOG(LogGeneral, Error, TEXT("%s"), *Message);
    break;

  case EPrintLogLevel::Info:
  default:
    UE_LOG(LogGeneral, Log, TEXT("%s"), *Message);
    break;
  }
}

void UGeneralFunctionLibrary::SetInputMode(APlayerController* PlayerController, EViewportInputMode Mode) {
  if (!IsValid(PlayerController)) return;

  switch (Mode) {
  case EViewportInputMode::GameOnly:
    PlayerController->SetInputMode(FInputModeGameOnly());
    PlayerController->bShowMouseCursor = false;
    break;

  case EViewportInputMode::UIOnly:
    PlayerController->SetInputMode(FInputModeUIOnly());
    PlayerController->bShowMouseCursor = true;
    break;

  case EViewportInputMode::GameAndUI:
  default:
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    PlayerController->SetInputMode(InputMode);
    PlayerController->bShowMouseCursor = true;
    break;
  }
}
