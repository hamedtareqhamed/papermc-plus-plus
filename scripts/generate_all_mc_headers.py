import json
import os
import glob
from collections import defaultdict

def load_json(path):
    with open(path, 'r') as f:
        return json.load(f)

def flatten_tags(tag_dict, tag_name, resolved, resolving):
    if tag_name in resolved:
        return resolved[tag_name]
    
    if tag_name in resolving:
        raise Exception(f"Circular tag reference detected involving {tag_name}")
    
    resolving.add(tag_name)
    result = []
    
    if tag_name in tag_dict:
        for val in tag_dict[tag_name]:
            if val.startswith('#'):
                result.extend(flatten_tags(tag_dict, val[1:], resolved, resolving))
            else:
                result.append(val)
                
    resolved[tag_name] = result
    resolving.remove(tag_name)
    return result

import re

VALID_ID_PATTERN = re.compile(r'^[a-z0-9_.-]+$')

def is_valid_entry(entry_name):
    path = entry_name.split(':', 1)[-1]
    return bool(VALID_ID_PATTERN.match(path))

def generate_headers():
    reports_dir = '/home/hamed/projectes/papermc++/mc/generated/reports'
    tags_dir = '/home/hamed/projectes/papermc++/mc/generated/data/minecraft/tags'
    out_dir = '/home/hamed/projectes/papermc++/include/core/protocol/generated'
    os.makedirs(out_dir, exist_ok=True)
    
    # 1. Load registries
    all_registries = {}
    reg_path = os.path.join(reports_dir, 'registries.json')
    if os.path.exists(reg_path):
        all_registries.update(load_json(reg_path))
            
    dp_path = os.path.join(reports_dir, 'datapack.json')
    if os.path.exists(dp_path):
        dp = load_json(dp_path)
        if 'registries' in dp:
            for k, v in dp['registries'].items():
                if k not in all_registries:
                    all_registries[k] = v

    registry_id_map = {}
    
    # Parse all registries
    for reg_name, reg_data in all_registries.items():
        reg_map = {}
        if "entries" in reg_data:
            entries = reg_data["entries"]
            if isinstance(entries, dict):
                for entry_k, entry_v in entries.items():
                    if not is_valid_entry(entry_k):
                        continue
                    if "protocol_id" in entry_v:
                        reg_map[entry_k] = entry_v["protocol_id"]
                    else:
                        reg_map[entry_k] = len(reg_map)
            elif isinstance(entries, list):
                for i, entry_k in enumerate(entries):
                    if is_valid_entry(entry_k):
                        reg_map[entry_k] = i
        registry_id_map[reg_name] = reg_map

    FALLBACK_ENTRIES = {
        "minecraft:dimension_type": ["minecraft:overworld"],
        "minecraft:biome": ["minecraft:plains"],
        "minecraft:cat_variant": ["minecraft:all_black", "minecraft:black", "minecraft:british_shorthair", "minecraft:calico", "minecraft:jellie", "minecraft:persian", "minecraft:ragdoll", "minecraft:red", "minecraft:siamese", "minecraft:tabby", "minecraft:white"],
        "minecraft:chicken_variant": ["minecraft:cold", "minecraft:temperate", "minecraft:warm"],
        "minecraft:cow_variant": ["minecraft:cold", "minecraft:temperate", "minecraft:warm"],
        "minecraft:frog_variant": ["minecraft:cold", "minecraft:temperate", "minecraft:warm"],
        "minecraft:painting_variant": ["minecraft:kebab"],
        "minecraft:pig_variant": ["minecraft:cold", "minecraft:temperate", "minecraft:warm"],
        "minecraft:wolf_variant": ["minecraft:ashen", "minecraft:black", "minecraft:chestnut", "minecraft:pale", "minecraft:rusty", "minecraft:snowy", "minecraft:spotted", "minecraft:striped", "minecraft:woods"],
        "minecraft:zombie_nautilus_variant": ["minecraft:temperate"],
        "minecraft:cat_sound_variant": ["minecraft:classic"],
        "minecraft:chicken_sound_variant": ["minecraft:classic"],
        "minecraft:cow_sound_variant": ["minecraft:classic"],
        "minecraft:pig_sound_variant": ["minecraft:classic"],
        "minecraft:wolf_sound_variant": ["minecraft:classic"],
        "minecraft:trim_material": ["minecraft:quartz", "minecraft:iron", "minecraft:netherite", "minecraft:redstone", "minecraft:copper", "minecraft:gold", "minecraft:emerald", "minecraft:diamond", "minecraft:lapis", "minecraft:amethyst", "minecraft:resin"],
        "minecraft:trim_pattern": ["minecraft:sentry", "minecraft:dune", "minecraft:coast", "minecraft:wild", "minecraft:ward", "minecraft:eye", "minecraft:vex", "minecraft:tide", "minecraft:snout", "minecraft:rib", "minecraft:spire", "minecraft:wayfinder", "minecraft:raiser", "minecraft:shaper", "minecraft:host", "minecraft:silence", "minecraft:flow", "minecraft:bolt"],
        "minecraft:jukebox_song": ["minecraft:11", "minecraft:13", "minecraft:5", "minecraft:blocks", "minecraft:bounce", "minecraft:cat", "minecraft:chirp", "minecraft:creator", "minecraft:creator_music_box", "minecraft:far", "minecraft:lava_chicken", "minecraft:mall", "minecraft:mellohi", "minecraft:otherside", "minecraft:pigstep", "minecraft:precipice", "minecraft:relic", "minecraft:stal", "minecraft:strad", "minecraft:tears", "minecraft:wait", "minecraft:ward"],
        "minecraft:instrument": ["minecraft:ponder_goat_horn", "minecraft:sing_goat_horn", "minecraft:seek_goat_horn", "minecraft:feel_goat_horn", "minecraft:admire_goat_horn", "minecraft:call_goat_horn", "minecraft:yearn_goat_horn", "minecraft:dream_goat_horn"],
        "minecraft:banner_pattern": ["minecraft:base", "minecraft:square_bottom_left", "minecraft:square_bottom_right", "minecraft:square_top_left", "minecraft:square_top_right", "minecraft:stripe_bottom", "minecraft:stripe_top", "minecraft:stripe_left", "minecraft:stripe_right", "minecraft:stripe_center", "minecraft:stripe_middle", "minecraft:stripe_downright", "minecraft:stripe_downleft", "minecraft:small_stripes", "minecraft:cross", "minecraft:straight_cross", "minecraft:triangle_bottom", "minecraft:triangle_top", "minecraft:triangles_bottom", "minecraft:triangles_top", "minecraft:diagonal_left", "minecraft:diagonal_right", "minecraft:diagonal_up_left", "minecraft:diagonal_up_right", "minecraft:circle", "minecraft:rhombus", "minecraft:half_vertical", "minecraft:half_horizontal", "minecraft:half_vertical_right", "minecraft:half_horizontal_bottom", "minecraft:border", "minecraft:curly_border", "minecraft:gradient", "minecraft:gradient_up", "minecraft:bricks", "minecraft:globe", "minecraft:creeper", "minecraft:skull", "minecraft:flower", "minecraft:mojang", "minecraft:piglin", "minecraft:flow", "minecraft:guster"],
        "minecraft:timeline": ["minecraft:overworld"],
        "minecraft:world_clock": ["minecraft:overworld"]
    }

    # Populate fallback entries for registries with missing/incomplete entries
    for reg_name, fallback_list in FALLBACK_ENTRIES.items():
        if reg_name not in registry_id_map or len(registry_id_map[reg_name]) < len(fallback_list):
            registry_id_map[reg_name] = {entry: i for i, entry in enumerate(fallback_list)}

    # 1. Generate registries.hpp
    reg_content = [
        "// AUTOGENERATED FILE - DO NOT EDIT",
        "#ifndef PAPERMC_CORE_PROTOCOL_GENERATED_REGISTRIES_HPP",
        "#define PAPERMC_CORE_PROTOCOL_GENERATED_REGISTRIES_HPP",
        "",
        "#include <string>",
        "#include <vector>",
        "#include <map>",
        "#include <cstdint>",
        "",
        "namespace papermc::core::protocol::generated {",
        "",
        "    struct RegistryData {",
        "        std::string name;",
        "        std::vector<std::string> entries;",
        "        bool include_overworld_nbt;",
        "    };",
        "",
        "    inline const std::vector<RegistryData> all_registries = {"
    ]
    DYNAMIC_REGISTRY_NAMES = {
        "minecraft:damage_type",
        "minecraft:dimension_type",
        "minecraft:biome",
        "minecraft:timeline",
        "minecraft:world_clock",
        "minecraft:cat_variant",
        "minecraft:chicken_variant",
        "minecraft:cow_variant",
        "minecraft:frog_variant",
        "minecraft:painting_variant",
        "minecraft:pig_variant",
        "minecraft:wolf_variant",
        "minecraft:zombie_nautilus_variant",
        "minecraft:cat_sound_variant",
        "minecraft:chicken_sound_variant",
        "minecraft:cow_sound_variant",
        "minecraft:pig_sound_variant",
        "minecraft:wolf_sound_variant",
        "minecraft:trim_material",
        "minecraft:trim_pattern",
        "minecraft:jukebox_song",
        "minecraft:instrument",
        "minecraft:banner_pattern"
    }

    for reg_name, reg_map in registry_id_map.items():
        if len(reg_map) == 0 or reg_name not in DYNAMIC_REGISTRY_NAMES: continue
        include_nbt = "false"
        reg_content.append(f"        {{\"{reg_name}\", {{")
        
        # Sort by protocol id
        sorted_entries = sorted(reg_map.items(), key=lambda x: x[1])
        for entry_name, _ in sorted_entries:
            if is_valid_entry(entry_name):
                reg_content.append(f"            \"{entry_name}\",")
        reg_content.append(f"        }}, {include_nbt}}},")

    reg_content.extend([
        "    };",
        "}",
        "#endif"
    ])
    with open(os.path.join(out_dir, 'registries.hpp'), 'w') as f:
        f.write("\n".join(reg_content) + "\n")


    # 2. Generate tags.hpp
    all_tags = {}
    for root, dirs, files in os.walk(tags_dir):
        for file in files:
            if not file.endswith('.json'):
                continue
            file_path = os.path.join(root, file)
            rel_path = os.path.relpath(file_path, tags_dir)
            parts = rel_path.split(os.sep)
            
            if len(parts) >= 2:
                matched_category = None
                for i in range(len(parts), 0, -1):
                    possible_cat = '/'.join(parts[:i])
                    if f"minecraft:{possible_cat}" in registry_id_map or possible_cat in ["block", "item", "entity_type", "fluid", "game_event"]:
                        matched_category = possible_cat
                        break
                if not matched_category:
                    matched_category = parts[0]
            else:
                matched_category = parts[0]
                
            reg_name = f"minecraft:{matched_category}"
            if reg_name not in all_tags:
                all_tags[reg_name] = {}
                
            tag_rel_path = '/'.join(parts[len(matched_category.split('/')):])
            tag_name = f"minecraft:{tag_rel_path[:-5]}"
            
            tag_data = load_json(file_path)
            values = []
            for val in tag_data.get('values', []):
                if isinstance(val, dict):
                    if val.get('required', True) == False:
                        continue
                    values.append(val['id'])
                else:
                    values.append(val)
            all_tags[reg_name][tag_name] = values

    tag_structs = []
    for reg_name, tags in all_tags.items():
        if reg_name not in registry_id_map or len(registry_id_map[reg_name]) == 0:
            continue
        resolved = {}
        resolving = set()
        reg_id_map = registry_id_map[reg_name]
        for tag_name in tags.keys():
            flatten_tags(tags, tag_name, resolved, resolving)
        tag_str = f"        {{\"{reg_name}\", {{\n"
        tag_entries = []
        for tag_name, entries in resolved.items():
            numerical_ids = []
            for e in entries:
                if e in reg_id_map:
                    numerical_ids.append(reg_id_map[e])
                else:
                    e_mc = e if e.startswith('minecraft:') else f"minecraft:{e}"
                    if e_mc in reg_id_map:
                        numerical_ids.append(reg_id_map[e_mc])
            numerical_ids = list(dict.fromkeys(numerical_ids))
            if numerical_ids:
                ids_str = ", ".join(map(str, numerical_ids))
                tag_entries.append(f"            {{\"{tag_name}\", {{{ids_str}}}}}")
            else:
                tag_entries.append(f"            {{\"{tag_name}\", {{}}}}")
        tag_str += ",\n".join(tag_entries)
        tag_str += "\n        }}"
        tag_structs.append(tag_str)

    tag_content = [
        "// AUTOGENERATED FILE - DO NOT EDIT",
        "#ifndef PAPERMC_CORE_PROTOCOL_GENERATED_TAGS_HPP",
        "#define PAPERMC_CORE_PROTOCOL_GENERATED_TAGS_HPP",
        "",
        "#include <string>",
        "#include <vector>",
        "#include <cstdint>",
        "",
        "namespace papermc::core::protocol::generated {",
        "    inline const std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::vector<int32_t>>>>> registry_tags = {",
        ",\n".join(tag_structs),
        "    };",
        "}",
        "#endif"
    ]
    with open(os.path.join(out_dir, 'tags.hpp'), 'w') as f:
        f.write("\n".join(tag_content) + "\n")

    # 3. Generate packets.hpp
    pack_path = os.path.join(reports_dir, 'packets.json')
    pack_content = [
        "// AUTOGENERATED FILE - DO NOT EDIT",
        "#ifndef PAPERMC_CORE_PROTOCOL_GENERATED_PACKETS_HPP",
        "#define PAPERMC_CORE_PROTOCOL_GENERATED_PACKETS_HPP",
        "",
        "#include <string>",
        "#include <cstdint>",
        "",
        "namespace papermc::core::protocol::generated {",
        "    struct PacketIds {"
    ]
    if os.path.exists(pack_path):
        packets_data = load_json(pack_path)
        for state, state_data in packets_data.items():
            for direction, dir_data in state_data.items():
                struct_name = f"{state.capitalize()}{direction.capitalize()}"
                pack_content.append(f"        struct {struct_name} {{")
                for packet_name, packet_info in dir_data.items():
                    name_clean = packet_name.replace("minecraft:", "")
                    pack_content.append(f"            static constexpr int32_t {name_clean} = {packet_info['protocol_id']};")
                pack_content.append("        };")
    pack_content.extend([
        "    };",
        "}",
        "#endif"
    ])
    with open(os.path.join(out_dir, 'packets.hpp'), 'w') as f:
        f.write("\n".join(pack_content) + "\n")


    # 4. Generate blocks.hpp
    blocks_path = os.path.join(reports_dir, 'blocks.json')
    blocks_content = [
        "// AUTOGENERATED FILE - DO NOT EDIT",
        "#ifndef PAPERMC_CORE_PROTOCOL_GENERATED_BLOCKS_HPP",
        "#define PAPERMC_CORE_PROTOCOL_GENERATED_BLOCKS_HPP",
        "",
        "#include <string>",
        "#include <cstdint>",
        "#include <map>",
        "",
        "namespace papermc::core::protocol::generated {",
        "    inline const std::map<std::string, int32_t> block_default_states = {"
    ]
    if os.path.exists(blocks_path):
        blocks_data = load_json(blocks_path)
        for block_name, block_info in blocks_data.items():
            default_id = 0
            for state in block_info.get("states", []):
                if state.get("default", False):
                    default_id = state["id"]
                    break
            blocks_content.append(f"        {{\"{block_name}\", {default_id}}},")
    blocks_content.extend([
        "    };",
        "}",
        "#endif"
    ])
    with open(os.path.join(out_dir, 'blocks.hpp'), 'w') as f:
        f.write("\n".join(blocks_content) + "\n")

if __name__ == '__main__':
    generate_headers()
