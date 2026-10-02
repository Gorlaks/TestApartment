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
    return ParseError(TEXT("JSON root must be an object."));
  }

  const TSharedPtr<FJsonObject>* BuildingObject = nullptr;
  if (!Root->TryGetObjectField(TEXT("building"), BuildingObject) || !BuildingObject || !BuildingObject->IsValid()) {
    return ParseError(TEXT("Missing building object."));
  }

  // При неверном обязательном поле, работа прерывается и новые данные не отдаются
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

  // Далее также идут проверки этажей и квартир
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
    if (!FloorObject.IsValid()) {
      return ParseError(FString::Printf(TEXT("floors[%d] must be an object."), FloorIndex));
    }

    if (!ReadString(*FloorObject, TEXT("id"), Floor.Id)) {
      return ParseError(FString::Printf(TEXT("Invalid id at floors[%d]."), FloorIndex));
    }

    double Number = 0.0;
    if (!ReadNumber(*FloorObject, TEXT("number"), Number)
      || Number < 1.0 || Number > MAX_int32) {
      return ParseError(FString::Printf(TEXT("Invalid number at floors[%d]."), FloorIndex));
    }
    if (static_cast<int32>(Number) != Number) {
      return ParseError(FString::Printf(TEXT("Floor number must be an integer at floors[%d]."), FloorIndex));
    }
    Floor.Number = static_cast<int32>(Number);

    if (!ReadVector(*FloorObject, TEXT("focus_point"), Floor.FocusPoint)) {
      return ParseError(FString::Printf(TEXT("Invalid focus_point at floors[%d]."), FloorIndex));
    }

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
      const FString ApartmentPath = FString::Printf(
        TEXT("floors[%d].apartments[%d]"), FloorIndex, ApartmentIndex);
      const TSharedPtr<FJsonValue>& ApartmentValue = (*Apartments)[ApartmentIndex];
      if (!ApartmentValue.IsValid() || ApartmentValue->Type != EJson::Object) {
        return ParseError(ApartmentPath + TEXT(" must be an object"));
      }

      const TSharedPtr<FJsonObject> ApartmentObject = ApartmentValue->AsObject();
      if (!ApartmentObject.IsValid()) {
        return ParseError(ApartmentPath + TEXT("must be an object"));
      }

      FUnitData Apartment;
      if (!ReadString(*ApartmentObject, TEXT("id"), Apartment.Id)) {
        return ParseError(ApartmentPath + TEXT("id is empty"));
      }

      FString Status;
      if (!ReadString(*ApartmentObject, TEXT("status"), Status)) {
        return ParseError(ApartmentPath + TEXT("status is empty"));
      }

      if (Status.Equals(TEXT("free"), ESearchCase::IgnoreCase)) {
        Apartment.Status = EUnitStatus::Free;
      } else if (Status.Equals(TEXT("sold"), ESearchCase::IgnoreCase)) {
        Apartment.Status = EUnitStatus::Sold;
      } else {
        return ParseError(ApartmentPath + TEXT("status must be free or sold"));
      }

      if (!ReadNumber(*ApartmentObject, TEXT("area_sqm"), Apartment.AreaSqm)
        || Apartment.AreaSqm <= 0.0) {
        return ParseError(ApartmentPath + TEXT("area_sqm must be a positive number"));
      }

      if (!ReadVector(*ApartmentObject, TEXT("focus_point"), Apartment.FocusPoint)) {
        return ParseError(ApartmentPath + TEXT("focus_point must contain numeric x, y and z"));
      }

      if (ApartmentIds.Contains(Apartment.Id)) {
        return ParseError(FString::Printf(TEXT("Duplicate apartment id '%s'"), *Apartment.Id));
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
  Result.Message = FString::Printf(TEXT("Loaded"));
  return Result;
}
