#include "DataSubsystem.h"

#include "General/FunctionLibrary.h"
#include "General/JsonParser.h"
#include "Async/Async.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UDataSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
  Super::Initialize(Collection);
  LoadDefaultData();
}

bool UDataSubsystem::LoadDefaultData() {
  FString FilePath = FPaths::Combine(
    FPaths::ProjectDir(), TEXT("Source"), TEXT("Data"), TEXT("config.json"));
  FilePath.TrimStartAndEndInline();
  return LoadFromFile(FilePath);
}
//TODO
// Load data from Json file
bool UDataSubsystem::LoadFromFile(const FString& FilePath) {
  if (bIsLoading) return false;

  if (FilePath.IsEmpty()) {
    UGeneralFunctionLibrary::PrintLog(TEXT("JSON file path is empty."), EPrintLogLevel::Error);
    OnLoadFinished.Broadcast(false, TEXT("JSON file path is empty."));
    return false;
  }

  const FString AbsolutePath = FPaths::ConvertRelativePathToFull(FilePath);
  const TWeakObjectPtr<UDataSubsystem> WeakThis(this);
  bIsLoading = true;

  Async(EAsyncExecution::ThreadPool, [WeakThis, AbsolutePath]() {
    FConfigParseResult Result;
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *AbsolutePath)) {
      Result.Message = FString::Printf(TEXT("Could not read JSON file: %s"), *AbsolutePath);
    } else {
      Result = FJsonParser::Parse(JsonText);
    }

    AsyncTask(ENamedThreads::GameThread, [WeakThis, Result = MoveTemp(Result)]() mutable {
      UDataSubsystem* Subsystem = WeakThis.Get();
      if (!Subsystem || !Subsystem->bIsLoading) return;

      if (Result.bSuccess) {
        Subsystem->BuildingData = MoveTemp(Result.Building);
        Subsystem->bHasData = true;
        UGeneralFunctionLibrary::PrintLog(Result.Message);
      } else {
        UGeneralFunctionLibrary::PrintLog(Result.Message, EPrintLogLevel::Error);
      }

      Subsystem->bIsLoading = false;
      Subsystem->OnLoadFinished.Broadcast(Result.bSuccess, Result.Message);
    });
  });

  return true;
}

FBuildingData UDataSubsystem::GetBuildingData() const {
  return bHasData ? BuildingData : FBuildingData{};
}

void UDataSubsystem::Deinitialize() {
  bIsLoading = false;
  Super::Deinitialize();
}
