# Обновляет точки фокуса квартир и этажей по BP_Unit в открытом уровне.
import json
import os

import unreal


def main():
  config_path = os.path.join(unreal.Paths.project_dir(), "Source", "Data", "config.json")
  with open(config_path, "r", encoding="utf-8") as file:
    config = json.load(file)

  apartments = {}
  for floor in config["building"]["floors"]:
    for apartment in floor["apartments"]:
      unit_id = str(apartment["id"]).strip()
      if unit_id in apartments:
        raise ValueError(f"Повторяющийся ID в JSON: {unit_id}")
      apartments[unit_id] = apartment

  positions = {}
  editor_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
  for actor in editor_actors.get_all_level_actors():
    unit = actor.get_component_by_class(unreal.UnitComponent)
    if unit is None:
      continue

    unit_id = str(unit.get_editor_property("unit_id")).strip()
    if not unit_id:
      raise ValueError(f"У {actor.get_actor_label()} не заполнен UnitId")
    if unit_id not in apartments:
      raise ValueError(f"ID {unit_id} у {actor.get_actor_label()} отсутствует в JSON")
    if unit_id in positions:
      raise ValueError(f"В уровне несколько квартир с ID {unit_id}")

    # Корень BP_Unit может стоять у пола. Камере нужен центр комнаты.
    location, extent = actor.get_actor_bounds(False)
    positions[unit_id] = {
      "x": round(float(location.x), 2),
      "y": round(float(location.y), 2),
      "z": round(float(location.z), 2),
    }
    unreal.log(
      f"Квартира {unit_id}: корень Z={actor.get_actor_location().z:.2f}, "
      f"центр Z={location.z:.2f}, высота границ={2 * extent.z:.2f}"
    )

  if not positions:
    raise ValueError("В открытом уровне не найдено акторов с UnitComponent")

  for unit_id, position in positions.items():
    apartments[unit_id]["focus_point"] = position
    unreal.log(f"Квартира {unit_id}: {position}")

  for floor in config["building"]["floors"]:
    unit_ids = [str(apartment["id"]).strip() for apartment in floor["apartments"]]
    if all(unit_id in positions for unit_id in unit_ids):
      floor["focus_point"] = {
        axis: round(sum(positions[unit_id][axis] for unit_id in unit_ids) / len(unit_ids), 2)
        for axis in ("x", "y", "z")
      }
      unreal.log(f"Этаж {floor['id']}: {floor['focus_point']}")

  if all(unit_id in positions for unit_id in apartments):
    config["building"]["genplan_focus_point"] = {
      axis: round(sum(position[axis] for position in positions.values()) / len(positions), 2)
      for axis in ("x", "y", "z")
    }
    unreal.log(f"Общий вид: {config['building']['genplan_focus_point']}")

  temp_path = config_path + ".tmp"
  with open(temp_path, "w", encoding="utf-8", newline="\n") as file:
    json.dump(config, file, ensure_ascii=False, indent=2)
    file.write("\n")
  os.replace(temp_path, config_path)

  unreal.log(f"Обновлено квартир: {len(positions)}. Файл: {config_path}")
  missing = sorted(set(apartments) - set(positions))
  if missing:
    unreal.log_warning("Без акторов в открытом уровне: " + ", ".join(missing))


main()
