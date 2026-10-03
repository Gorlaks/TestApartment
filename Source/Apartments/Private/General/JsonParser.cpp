#include "General/JsonParser.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace {
// UE умеет преобразовывать строки и числа друг в друга, поэтому проверяем тип явно
bool ReadString(const FJsonObject& Object, const TCHAR* Field, FString& Value) {
  const TSharedPtr<FJsonValue> JsonValue = Object.TryGetField(Field);
  if (!JsonValue.IsValid() || JsonValue->Type != EJson::String) return false;
  if (!Object.TryGetStringField(Field, Value)) return false;
  Value.TrimStartAndEndInline();
  return !Value.IsEmpty();
}

bool ReadNumber(const FJsonObject& Object, const TCHAR* Field, double& Value) {
  const TSharedPtr<FJsonValue> JsonValue = Object.TryGetField(Field);
  return JsonValue.IsValid() && JsonValue->Type == EJson::Number
    && JsonValue->TryGetNumber(Value) && FMath::IsFinite(Value);
}

bool ReadVector(const FJsonObject& Object, const TCHAR* Field, FVector& Value) {
  const TSharedPtr<FJsonObject>* VectorObject = nullptr;
  if (!Object.TryGetObjectField(Field, VectorObject) || !VectorObject || !VectorObject->IsValid()) return false;

  double X, Y, Z;
  if (!ReadNumber(**VectorObject, TEXT("x"), X)
    || !ReadNumber(**VectorObject, TEXT("y"), Y)
    || !ReadNumber(**VectorObject, TEXT("z"), Z)) return false;

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
    return ParseError(TEXT("JSON root must be an object"));
  }

  const TSharedPtr<FJsonObject>* BuildingObject = nullptr;
  if (!Root->TryGetObjectField(TEXT("building"), BuildingObject) || !BuildingObject || !BuildingObject->IsValid()) {
    return ParseError(TEXT("Missing building object"));
  }

  // Без точки общего вида камера не сможет показать здание
  FBuildingData Building;
  if (!ReadString(**BuildingObject, TEXT("id"), Building.Id)
    || !ReadString(**BuildingObject, TEXT("name"), Building.Name)
    || !ReadVector(**BuildingObject, TEXT("genplan_focus_point"), Building.GenplanFocusPoint)) {
    return ParseError(TEXT("Invalid building: check id, name and genplan_focus_point"));
  }

  const TArray<TSharedPtr<FJsonValue>>* Floors = nullptr;
  if (!(*BuildingObject)->TryGetArrayField(TEXT("floors"), Floors) || !Floors || Floors->IsEmpty()) {
    return ParseError(TEXT("building.floors must be a non-empty array"));
  }

  TArray<FString> Warnings;
  TSet<FString> FloorIds;
  TSet<int32> FloorNumbers;
  TSet<FString> ApartmentIds;

  for (int32 FloorIndex = 0; FloorIndex < Floors->Num(); ++FloorIndex) {
    const TSharedPtr<FJsonValue>& FloorValue = (*Floors)[FloorIndex];
    if (!FloorValue.IsValid() || FloorValue->Type != EJson::Object) {
      Warnings.Add(FString::Printf(TEXT("floors[%d] must be an object"), FloorIndex));
      continue;
    }

    const TSharedPtr<FJsonObject> FloorObject = FloorValue->AsObject();
    FFloorData Floor;
    if (!FloorObject.IsValid()) {
      Warnings.Add(FString::Printf(TEXT("floors[%d] must be an object"), FloorIndex));
      continue;
    }

    if (!ReadString(*FloorObject, TEXT("id"), Floor.Id)) {
      Warnings.Add(FString::Printf(TEXT("Invalid id at floors[%d]"), FloorIndex));
      continue;
    }

    double Number = 0.0;
    if (!ReadNumber(*FloorObject, TEXT("number"), Number)
      || Number < 1.0 || Number > MAX_int32) {
      Warnings.Add(FString::Printf(TEXT("Invalid number at floors[%d]"), FloorIndex));
      continue;
    }
    if (static_cast<int32>(Number) != Number) {
      Warnings.Add(FString::Printf(TEXT("Floor number must be an integer at floors[%d]"), FloorIndex));
      continue;
    }
    Floor.Number = static_cast<int32>(Number);

    if (!ReadVector(*FloorObject, TEXT("focus_point"), Floor.FocusPoint)) {
      Warnings.Add(FString::Printf(TEXT("Invalid focus_point at floors[%d]"), FloorIndex));
      continue;
    }

    if (FloorIds.Contains(Floor.Id) || FloorNumbers.Contains(Floor.Number)) {
      Warnings.Add(FString::Printf(TEXT("Duplicate floor id or number at floors[%d]"), FloorIndex));
      continue;
    }
    FloorIds.Add(Floor.Id);
    FloorNumbers.Add(Floor.Number);

    const TArray<TSharedPtr<FJsonValue>>* Apartments = nullptr;
    if (!FloorObject->TryGetArrayField(TEXT("apartments"), Apartments) || !Apartments) {
      Warnings.Add(FString::Printf(TEXT("floors[%d].apartments must be an array"), FloorIndex));
      Building.Floors.Add(MoveTemp(Floor));
      continue;
    }

    for (int32 ApartmentIndex = 0; ApartmentIndex < Apartments->Num(); ++ApartmentIndex) {
      const FString ApartmentPath = FString::Printf(
        TEXT("floors[%d].apartments[%d]"), FloorIndex, ApartmentIndex);
      const TSharedPtr<FJsonValue>& ApartmentValue = (*Apartments)[ApartmentIndex];
      if (!ApartmentValue.IsValid() || ApartmentValue->Type != EJson::Object) {
        Warnings.Add(ApartmentPath + TEXT(" must be an object"));
        continue;
      }

      const TSharedPtr<FJsonObject> ApartmentObject = ApartmentValue->AsObject();
      if (!ApartmentObject.IsValid()) {
        Warnings.Add(ApartmentPath + TEXT(" must be an object"));
        continue;
      }

      FUnitData Apartment;
      if (!ReadString(*ApartmentObject, TEXT("id"), Apartment.Id)) {
        Warnings.Add(ApartmentPath + TEXT(": id is missing or empty"));
        continue;
      }

      FString Status;
      if (!ReadString(*ApartmentObject, TEXT("status"), Status)) {
        Warnings.Add(ApartmentPath + TEXT(": status is missing or empty"));
        continue;
      }

      if (Status.Equals(TEXT("free"), ESearchCase::IgnoreCase)) {
        Apartment.Status = EUnitStatus::Free;
      } else if (Status.Equals(TEXT("sold"), ESearchCase::IgnoreCase)) {
        Apartment.Status = EUnitStatus::Sold;
      } else {
        Warnings.Add(ApartmentPath + TEXT(": status must be free or sold"));
        continue;
      }

      if (!ReadNumber(*ApartmentObject, TEXT("area_sqm"), Apartment.AreaSqm)
        || Apartment.AreaSqm <= 0.0) {
        Warnings.Add(ApartmentPath + TEXT(": area_sqm must be a positive number"));
        continue;
      }

      if (!ReadVector(*ApartmentObject, TEXT("focus_point"), Apartment.FocusPoint)) {
        Warnings.Add(ApartmentPath + TEXT(": focus_point must contain numeric x, y and z"));
        continue;
      }

      if (ApartmentIds.Contains(Apartment.Id)) {
        Warnings.Add(FString::Printf(TEXT("Duplicate apartment id '%s'"), *Apartment.Id));
        continue;
      }
      ApartmentIds.Add(Apartment.Id);
      Floor.Apartments.Add(MoveTemp(Apartment));
    }

    Building.Floors.Add(MoveTemp(Floor));
  }

  FConfigParseResult Result;
  Result.bSuccess = true;
  Result.Building = MoveTemp(Building);
  Result.Message = TEXT("Data is loaded.");
  if (!Warnings.IsEmpty()) {
    Result.Message = FString::Printf(TEXT("Building data loaded with %d warning(s)"), Warnings.Num());
  }
  Result.Warnings = MoveTemp(Warnings);
  return Result;
}
