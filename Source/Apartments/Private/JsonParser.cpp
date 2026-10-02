#include "JsonParser.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace {
bool ReadString(const FJsonObject& Object, const TCHAR* Field, FString& Value) {
  if (!Object.TryGetStringField(Field, Value)) return false;
  Value.TrimStartAndEndInline();
  return !Value.IsEmpty();
}

bool ReadVector(const FJsonObject& Object, const TCHAR* Field, FVector& Value) {
  const TSharedPtr<FJsonObject>* VectorObject = nullptr;
  if (!Object.TryGetObjectField(Field, VectorObject) || !VectorObject || !VectorObject->IsValid()) return false;

  double X, Y, Z;
  if (!(*VectorObject)->TryGetNumberField(TEXT("x"), X)
    || !(*VectorObject)->TryGetNumberField(TEXT("y"), Y)
    || !(*VectorObject)->TryGetNumberField(TEXT("z"), Z)) return false;

  if (!FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Z)) return false;
  Value = FVector(X, Y, Z);
  return true;
}

FConfigParseResult ParseError(const FString& Message) {
  FConfigParseResult Result;
  Result.Message = Message;
  return Result;
}
}

FConfigParseResult FJsonParser::Parse(const FString& JsonText) {
  TSharedPtr<FJsonObject> Root;
  if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonText), Root) || !Root.IsValid()) {
    return ParseError(TEXT("JSON root must be an object."));
  }

  const TSharedPtr<FJsonObject>* BuildingObject = nullptr;
  if (!Root->TryGetObjectField(TEXT("building"), BuildingObject) || !BuildingObject || !BuildingObject->IsValid()) {
    return ParseError(TEXT("Missing building object."));
  }

  FBuildingData Building;
  if (!ReadString(**BuildingObject, TEXT("id"), Building.Id)
    || !ReadString(**BuildingObject, TEXT("name"), Building.Name)
    || !ReadVector(**BuildingObject, TEXT("genplan_focus_point"), Building.GenplanFocusPoint)) {
    return ParseError(TEXT("Invalid building: check id, name and genplan_focus_point."));
  }

  const TArray<TSharedPtr<FJsonValue>>* Floors = nullptr;
  if (!(*BuildingObject)->TryGetArrayField(TEXT("floors"), Floors) || !Floors || Floors->IsEmpty()) {
    return ParseError(TEXT("building.floors must be a non-empty array."));
  }

  TSet<FString> FloorIds;
  TSet<int32> FloorNumbers;
  TSet<FString> ApartmentIds;
  int32 ApartmentCount = 0;

  for (int32 FloorIndex = 0; FloorIndex < Floors->Num(); ++FloorIndex) {
    const TSharedPtr<FJsonValue>& FloorValue = (*Floors)[FloorIndex];
    if (!FloorValue.IsValid() || FloorValue->Type != EJson::Object) {
      return ParseError(FString::Printf(TEXT("floors[%d] must be an object."), FloorIndex));
    }

    const TSharedPtr<FJsonObject> FloorObject = FloorValue->AsObject();
    FFloorData Floor;
    double Number = 0.0;
    //TODO
    if (!FloorObject.IsValid()
      || !ReadString(*FloorObject, TEXT("id"), Floor.Id)
      || !FloorObject->TryGetNumberField(TEXT("number"), Number)
      || !FMath::IsFinite(Number) || Number < 1.0 || Number > MAX_int32
      || static_cast<int32>(Number) != Number
      || !ReadVector(*FloorObject, TEXT("focus_point"), Floor.FocusPoint)) {
      return ParseError(FString::Printf(TEXT("Invalid floors[%d]: check id, number and focus_point."), FloorIndex));
    }
    Floor.Number = static_cast<int32>(Number);

    if (FloorIds.Contains(Floor.Id) || FloorNumbers.Contains(Floor.Number)) {
      return ParseError(FString::Printf(TEXT("Duplicate floor id or number at floors[%d]."), FloorIndex));
    }
    FloorIds.Add(Floor.Id);
    FloorNumbers.Add(Floor.Number);

    const TArray<TSharedPtr<FJsonValue>>* Apartments = nullptr;
    if (!FloorObject->TryGetArrayField(TEXT("apartments"), Apartments) || !Apartments || Apartments->IsEmpty()) {
      return ParseError(FString::Printf(TEXT("floors[%d].apartments must be a non-empty array."), FloorIndex));
    }

    for (int32 ApartmentIndex = 0; ApartmentIndex < Apartments->Num(); ++ApartmentIndex) {
      const TSharedPtr<FJsonValue>& ApartmentValue = (*Apartments)[ApartmentIndex];
      if (!ApartmentValue.IsValid() || ApartmentValue->Type != EJson::Object) {
        return ParseError(FString::Printf(TEXT("floors[%d].apartments[%d] must be an object."), FloorIndex, ApartmentIndex));
      }

      const TSharedPtr<FJsonObject> ApartmentObject = ApartmentValue->AsObject();
      FUnitData Apartment;
      FString Status;
      if (!ApartmentObject.IsValid()
        || !ReadString(*ApartmentObject, TEXT("id"), Apartment.Id)
        || !ReadString(*ApartmentObject, TEXT("status"), Status)
        || !ApartmentObject->TryGetNumberField(TEXT("area_sqm"), Apartment.AreaSqm)
        || !FMath::IsFinite(Apartment.AreaSqm) || Apartment.AreaSqm <= 0.0
        || !ReadVector(*ApartmentObject, TEXT("focus_point"), Apartment.FocusPoint)) {
        return ParseError(FString::Printf(TEXT("Invalid floors[%d].apartments[%d]: check id, status, area_sqm and focus_point."), FloorIndex, ApartmentIndex));
      }

      if (Status.Equals(TEXT("free"), ESearchCase::IgnoreCase)) {
        Apartment.Status = EUnitStatus::Free;
      } else if (Status.Equals(TEXT("sold"), ESearchCase::IgnoreCase)) {
        Apartment.Status = EUnitStatus::Sold;
      } else {
        return ParseError(FString::Printf(TEXT("Unknown status '%s' at floors[%d].apartments[%d]."), *Status, FloorIndex, ApartmentIndex));
      }

      if (ApartmentIds.Contains(Apartment.Id)) {
        return ParseError(FString::Printf(TEXT("Duplicate apartment id '%s'."), *Apartment.Id));
      }
      ApartmentIds.Add(Apartment.Id);
      Floor.Apartments.Add(MoveTemp(Apartment));
      ++ApartmentCount;
    }

    Building.Floors.Add(MoveTemp(Floor));
  }

  FConfigParseResult Result;
  Result.bSuccess = true;
  Result.Building = MoveTemp(Building);
  Result.Message = FString::Printf(TEXT("Loaded %d floors and %d apartments."), Result.Building.Floors.Num(), ApartmentCount);
  return Result;
}
