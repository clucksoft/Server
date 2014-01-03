#ifdef LUA_EQEMU

#include "lua.hpp"
#include <luabind/luabind.hpp>
#include <luabind/object.hpp>

#include <ctype.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>

#include "masterentity.h"
#include "../common/spdat.h"
#include "lua_bit.h"
#include "lua_entity.h"
#include "lua_item.h"
#include "lua_iteminst.h"
#include "lua_mob.h"
#include "lua_hate_list.h"
#include "lua_client.h"
#include "lua_inventory.h"
#include "lua_npc.h"
#include "lua_spell.h"
#include "lua_entity_list.h"
//#include "lua_group.h"
//#include "lua_raid.h"
#include "lua_corpse.h"
#include "lua_object.h"
#include "lua_door.h"
#include "lua_spawn.h"
#include "lua_packet.h"
#include "lua_general.h"
#include "questmgr.h"
#include "zone.h"
#include "lua_parser.h"

const char *LuaEvents[_LargestEventID] = {
	"event_say",
	"event_trade",
	"event_death",
	"event_spawn",
	"event_attack",
	"event_combat",
	"event_aggro",
	"event_slay",
	"event_npc_slay",
	"event_waypoint_arrive",
	"event_waypoint_depart",
	"event_timer",
	"event_signal",
	"event_hp",
	"event_enter",
	"event_exit",
	"event_enter_zone",
	"event_click_door",
	"event_loot",
	"event_zone",
	"event_level_up",
	"event_killed_merit",
	"event_cast_on",
	"event_task_accepted",
	"event_task_stage_complete",
	"event_task_update",
	"event_task_complete",
	"event_task_fail",
	"event_aggro_say",
	"event_player_pickup",
	"event_popup_response",
	"event_proximity_say",
	"event_cast",
	"event_cast_begin",
	"event_scale_calc",
	"event_item_enter_zone",
	"event_target_change",
	"event_hate_list",
	"event_spell_effect",
	"event_spell_effect",
	"event_spell_buff_tic",
	"event_spell_buff_tic",
	"event_spell_fade",
	"event_spell_effect_translocate_complete",
	"event_combine_success",
	"event_combine_failure",
	"event_item_click",
	"event_item_click_cast",
	"event_group_change",
	"event_forage_success",
	"event_forage_failure",
	"event_fish_start",
	"event_fish_success",
	"event_fish_failure",
	"event_click_object",
	"event_discover_item",
	"event_disconnect",
	"event_connect",
	"event_item_tick",
	"event_duel_win",
	"event_duel_lose",
	"event_encounter_load",
	"event_encounter_unload",
	"event_command",
	"event_drop_item",
	"event_destroy_item",
	"event_feign_death",
	"event_weapon_proc",
	"event_equip_item",
	"event_unequip_item",
	"event_augment_item",
	"event_unaugment_item",
	"event_augment_insert",
	"event_augment_remove",
	"event_enter_area",
	"event_leave_area",
	"event_respawn",
	"event_death_complete",
	"event_unhandled_opcode"
};

extern Zone *zone;

struct lua_registered_event {
	std::string encounter_name;
	luabind::object lua_reference;
	QuestEventID event_id;
};

std::map<std::string, std::list<lua_registered_event>> lua_encounter_events_registered;

LuaParser::LuaParser() {
	for(int i = 0; i < _LargestEventID; ++i) {
		NPCArgumentDispatch[i] = handle_npc_null;
		PlayerArgumentDispatch[i] = handle_player_null;
		ItemArgumentDispatch[i] = handle_item_null;
		SpellArgumentDispatch[i] = handle_spell_null;
	}

	NPCArgumentDispatch[EVENT_SAY] = handle_npc_event_say;
	NPCArgumentDispatch[EVENT_AGGRO_SAY] = handle_npc_event_say;
	NPCArgumentDispatch[EVENT_PROXIMITY_SAY] = handle_npc_event_say;
	NPCArgumentDispatch[EVENT_TRADE] = handle_npc_event_trade;
	NPCArgumentDispatch[EVENT_HP] = handle_npc_event_hp;
	NPCArgumentDispatch[EVENT_TARGET_CHANGE] = handle_npc_single_mob;
	NPCArgumentDispatch[EVENT_CAST_ON] = handle_npc_cast;
	NPCArgumentDispatch[EVENT_KILLED_MERIT] = handle_npc_single_client;
	NPCArgumentDispatch[EVENT_SLAY] = handle_npc_single_mob;
	NPCArgumentDispatch[EVENT_ENTER] = handle_npc_single_client;
	NPCArgumentDispatch[EVENT_EXIT] = handle_npc_single_client;
	NPCArgumentDispatch[EVENT_TASK_ACCEPTED] = handle_npc_task_accepted;
	NPCArgumentDispatch[EVENT_POPUP_RESPONSE] = handle_npc_popup;
	NPCArgumentDispatch[EVENT_WAYPOINT_ARRIVE] = handle_npc_waypoint;
	NPCArgumentDispatch[EVENT_WAYPOINT_DEPART] = handle_npc_waypoint;
	NPCArgumentDispatch[EVENT_HATE_LIST] = handle_npc_hate;
	NPCArgumentDispatch[EVENT_COMBAT] = handle_npc_hate;
	NPCArgumentDispatch[EVENT_SIGNAL] = handle_npc_signal;
	NPCArgumentDispatch[EVENT_TIMER] = handle_npc_timer;
	NPCArgumentDispatch[EVENT_DEATH] = handle_npc_death;
	NPCArgumentDispatch[EVENT_DEATH_COMPLETE] = handle_npc_death;
	NPCArgumentDispatch[EVENT_CAST] = handle_npc_cast;
	NPCArgumentDispatch[EVENT_CAST_BEGIN] = handle_npc_cast;
	NPCArgumentDispatch[EVENT_FEIGN_DEATH] = handle_npc_single_client;
	NPCArgumentDispatch[EVENT_ENTER_AREA] = handle_npc_area;
	NPCArgumentDispatch[EVENT_LEAVE_AREA] = handle_npc_area;

	PlayerArgumentDispatch[EVENT_SAY] = handle_player_say;
	PlayerArgumentDispatch[EVENT_DEATH] = handle_player_death;
	PlayerArgumentDispatch[EVENT_DEATH_COMPLETE] = handle_player_death;
	PlayerArgumentDispatch[EVENT_TIMER] = handle_player_timer;
	PlayerArgumentDispatch[EVENT_DISCOVER_ITEM] = handle_player_discover_item;
	PlayerArgumentDispatch[EVENT_FISH_SUCCESS] = handle_player_fish_forage_success;
	PlayerArgumentDispatch[EVENT_FORAGE_SUCCESS] = handle_player_fish_forage_success;
	PlayerArgumentDispatch[EVENT_CLICK_OBJECT] = handle_player_click_object;
	PlayerArgumentDispatch[EVENT_CLICK_DOOR] = handle_player_click_door;
	PlayerArgumentDispatch[EVENT_SIGNAL] = handle_player_signal;
	PlayerArgumentDispatch[EVENT_POPUP_RESPONSE] = handle_player_popup_response;
	PlayerArgumentDispatch[EVENT_PLAYER_PICKUP] = handle_player_pick_up;
	PlayerArgumentDispatch[EVENT_CAST] = handle_player_cast;
	PlayerArgumentDispatch[EVENT_CAST_BEGIN] = handle_player_cast;
	PlayerArgumentDispatch[EVENT_TASK_FAIL] = handle_player_task_fail;
	PlayerArgumentDispatch[EVENT_ZONE] = handle_player_zone;
	PlayerArgumentDispatch[EVENT_DUEL_WIN] = handle_player_duel_win;
	PlayerArgumentDispatch[EVENT_DUEL_LOSE] = handle_player_duel_loss;
	PlayerArgumentDispatch[EVENT_LOOT] = handle_player_loot;
	PlayerArgumentDispatch[EVENT_TASK_STAGE_COMPLETE] = handle_player_task_stage_complete;
	PlayerArgumentDispatch[EVENT_TASK_COMPLETE] = handle_player_task_update;
	PlayerArgumentDispatch[EVENT_TASK_UPDATE] = handle_player_task_update;
	PlayerArgumentDispatch[EVENT_COMMAND] = handle_player_command;
	PlayerArgumentDispatch[EVENT_COMBINE_SUCCESS] = handle_player_combine;
	PlayerArgumentDispatch[EVENT_COMBINE_FAILURE] = handle_player_combine;
	PlayerArgumentDispatch[EVENT_FEIGN_DEATH] = handle_player_feign;
	PlayerArgumentDispatch[EVENT_ENTER_AREA] = handle_player_area;
	PlayerArgumentDispatch[EVENT_LEAVE_AREA] = handle_player_area;
	PlayerArgumentDispatch[EVENT_RESPAWN] = handle_player_respawn;
	PlayerArgumentDispatch[EVENT_UNHANDLED_OPCODE] = handle_player_packet;

	ItemArgumentDispatch[EVENT_ITEM_CLICK] = handle_item_click;
	ItemArgumentDispatch[EVENT_ITEM_CLICK_CAST] = handle_item_click;
	ItemArgumentDispatch[EVENT_TIMER] = handle_item_timer;
	ItemArgumentDispatch[EVENT_WEAPON_PROC] = handle_item_proc;
	ItemArgumentDispatch[EVENT_LOOT] = handle_item_loot;
	ItemArgumentDispatch[EVENT_EQUIP_ITEM] = handle_item_equip;
	ItemArgumentDispatch[EVENT_UNEQUIP_ITEM] = handle_item_equip;
	ItemArgumentDispatch[EVENT_AUGMENT_ITEM] = handle_item_augment;
	ItemArgumentDispatch[EVENT_UNAUGMENT_ITEM] = handle_item_augment;
	ItemArgumentDispatch[EVENT_AUGMENT_INSERT] = handle_item_augment_insert;
	ItemArgumentDispatch[EVENT_AUGMENT_REMOVE] = handle_item_augment_remove;

	SpellArgumentDispatch[EVENT_SPELL_EFFECT_CLIENT] = handle_spell_effect;
	SpellArgumentDispatch[EVENT_SPELL_BUFF_TIC_CLIENT] = handle_spell_tic;
	SpellArgumentDispatch[EVENT_SPELL_FADE] = handle_spell_fade;
	SpellArgumentDispatch[EVENT_SPELL_EFFECT_TRANSLOCATE_COMPLETE] = handle_translocate_finish;

	L = nullptr;
}

LuaParser::~LuaParser() {
	if(L) {
		lua_close(L);
	}
}

int LuaParser::EventNPC(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data,
						std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	if(!npc) {
		return 0;
	}

	if(!HasQuestSub(npc->GetNPCTypeID(), evt)) {
		return 0;
	}

	std::string package_name = "npc_" + std::to_string(static_cast<long long>(npc->GetNPCTypeID()));
	return _EventNPC(package_name, evt, npc, init, data, extra_data, extra_pointers);
}

int LuaParser::EventGlobalNPC(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data,
							  std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	if(!npc) {
		return 0;
	}

	if(!HasGlobalQuestSub(evt)) {
		return 0;
	}

	return _EventNPC("global_npc", evt, npc, init, data, extra_data, extra_pointers);
}

int LuaParser::_EventNPC(std::string package_name, QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data,
						 std::vector<void*> *extra_pointers, luabind::object *l_func) {
	const char *sub_name = LuaEvents[evt];

	int start = lua_gettop(L);

	try {
		int npop = 1;
		if(l_func != nullptr) {
			l_func->push(L);
		} else {
			lua_getfield(L, LUA_REGISTRYINDEX, package_name.c_str());
			lua_getfield(L, -1, sub_name);
			npop = 2;
		}
		
		lua_createtable(L, 0, 0);
		//always push self
		Lua_NPC l_npc(npc);
		luabind::object l_npc_o = luabind::object(L, l_npc);
		l_npc_o.push(L);
		lua_setfield(L, -2, "self");

		auto arg_function = NPCArgumentDispatch[evt];
		arg_function(this, L, npc, init, data, extra_data, extra_pointers);
		Client *c = (init && init->IsClient()) ? init->CastToClient() : nullptr;
		
		quest_manager.StartQuest(npc, c, nullptr);
		if(lua_pcall(L, 1, 1, 0)) {
			std::string error = lua_tostring(L, -1);
			AddError(error);
			quest_manager.EndQuest();
			return 0;
		}
		quest_manager.EndQuest();
		
		if(lua_isnumber(L, -1)) {
			int ret = static_cast<int>(lua_tointeger(L, -1));
			lua_pop(L, npop);
			return ret;
		}
		
		lua_pop(L, npop);
	} catch(std::exception &ex) {
		std::string error = "Lua Exception: ";
		error += std::string(ex.what());
		AddError(error);

		//Restore our stack to the best of our ability
		int end = lua_gettop(L);
		int n = end - start;
		if(n > 0) {
			lua_pop(L, n);
		}
	}

	return 0;
}

int LuaParser::EventPlayer(QuestEventID evt, Client *client, std::string data, uint32 extra_data,
		std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	if(!client) {
		return 0;
	}

	if(!PlayerHasQuestSub(evt)) {
		return 0;
	}

	return _EventPlayer("player", evt, client, data, extra_data, extra_pointers);
}

int LuaParser::EventGlobalPlayer(QuestEventID evt, Client *client, std::string data, uint32 extra_data,
		std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	if(!client) {
		return 0;
	}

	if(!GlobalPlayerHasQuestSub(evt)) {
		return 0;
	}

	return _EventPlayer("global_player", evt, client, data, extra_data, extra_pointers);
}

int LuaParser::_EventPlayer(std::string package_name, QuestEventID evt, Client *client, std::string data, uint32 extra_data,
							std::vector<void*> *extra_pointers, luabind::object *l_func) {
	const char *sub_name = LuaEvents[evt];
	int start = lua_gettop(L);

	try {
		int npop = 1;
		if(l_func != nullptr) {
			l_func->push(L);
		} else {
			lua_getfield(L, LUA_REGISTRYINDEX, package_name.c_str());
			lua_getfield(L, -1, sub_name);
			npop = 2;
		}
	
		lua_createtable(L, 0, 0);
		//push self
		Lua_Client l_client(client);
		luabind::object l_client_o = luabind::object(L, l_client);
		l_client_o.push(L);
		lua_setfield(L, -2, "self");
		
		auto arg_function = PlayerArgumentDispatch[evt];
		arg_function(this, L, client, data, extra_data, extra_pointers);
	
		quest_manager.StartQuest(client, client, nullptr);
		if(lua_pcall(L, 1, 1, 0)) {
			std::string error = lua_tostring(L, -1);
			AddError(error);
			quest_manager.EndQuest();
			return 0;
		}
		quest_manager.EndQuest();
		
		if(lua_isnumber(L, -1)) {
			int ret = static_cast<int>(lua_tointeger(L, -1));
			lua_pop(L, npop);
			return ret;
		}
		
		lua_pop(L, npop);
	} catch(std::exception &ex) {
		std::string error = "Lua Exception: ";
		error += std::string(ex.what());
		AddError(error);

		//Restore our stack to the best of our ability
		int end = lua_gettop(L);
		int n = end - start;
		if(n > 0) {
			lua_pop(L, n);
		}
	}

	return 0;
}

int LuaParser::EventItem(QuestEventID evt, Client *client, ItemInst *item, Mob *mob, std::string data, uint32 extra_data,
		std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}
	
	if(!item) {
		return 0;
	}
	
	if(!ItemHasQuestSub(item, evt)) {
		return 0;
	}
	
	std::string package_name = "item_";
	package_name += std::to_string(static_cast<long long>(item->GetID()));
	return _EventItem(package_name, evt, client, item, mob, data, extra_data, extra_pointers);
}

int LuaParser::_EventItem(std::string package_name, QuestEventID evt, Client *client, ItemInst *item, Mob *mob,
						  std::string data, uint32 extra_data, std::vector<void*> *extra_pointers, luabind::object *l_func) {
	const char *sub_name = LuaEvents[evt];

	int start = lua_gettop(L);

	try {
		int npop = 1;
		if(l_func != nullptr) {
			l_func->push(L);
		} else {
			lua_getfield(L, LUA_REGISTRYINDEX, package_name.c_str());
			lua_getfield(L, -1, sub_name);
		}
		
		lua_createtable(L, 0, 0);
		//always push self
		Lua_ItemInst l_item(item);
		luabind::object l_item_o = luabind::object(L, l_item);
		l_item_o.push(L);
		lua_setfield(L, -2, "self");

		Lua_Client l_client(client);
		luabind::object l_client_o = luabind::object(L, l_client);
		l_client_o.push(L);
		lua_setfield(L, -2, "owner");

		//redo this arg function
		auto arg_function = ItemArgumentDispatch[evt];
		arg_function(this, L, client, item, mob, data, extra_data, extra_pointers);
		
		quest_manager.StartQuest(client, client, item);
		if(lua_pcall(L, 1, 1, 0)) {
			std::string error = lua_tostring(L, -1);
			AddError(error);
			quest_manager.EndQuest();
			return 0;
		}
		quest_manager.EndQuest();
		
		if(lua_isnumber(L, -1)) {
			int ret = static_cast<int>(lua_tointeger(L, -1));
			lua_pop(L, npop);
			return ret;
		}
		
		lua_pop(L, npop);
	} catch(std::exception &ex) {
		std::string error = "Lua Exception: ";
		error += std::string(ex.what());
		AddError(error);

		//Restore our stack to the best of our ability
		int end = lua_gettop(L);
		int n = end - start;
		if(n > 0) {
			lua_pop(L, n);
		}
	}

	return 0;
}

int LuaParser::EventSpell(QuestEventID evt, NPC* npc, Client *client, uint32 spell_id, uint32 extra_data,
						  std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	std::string package_name = "spell_" + std::to_string(static_cast<long long>(spell_id));

	if(!SpellHasQuestSub(spell_id, evt)) {
		return 0;
	}
	
	return _EventSpell(package_name, evt, npc, client, spell_id, extra_data, extra_pointers);
}

int LuaParser::_EventSpell(std::string package_name, QuestEventID evt, NPC* npc, Client *client, uint32 spell_id, uint32 extra_data,
						   std::vector<void*> *extra_pointers, luabind::object *l_func) {
	const char *sub_name = LuaEvents[evt];
	
	int start = lua_gettop(L);

	try {
		int npop = 1;
		if(l_func != nullptr) {
			l_func->push(L);
		} else {
			lua_getfield(L, LUA_REGISTRYINDEX, package_name.c_str());
			lua_getfield(L, -1, sub_name);
			npop = 2;
		}
		
		lua_createtable(L, 0, 0);

		//always push self even if invalid
		if(IsValidSpell(spell_id)) {
			Lua_Spell l_spell(&spells[spell_id]);
			luabind::object l_spell_o = luabind::object(L, l_spell);
			l_spell_o.push(L);
		} else {
			Lua_Spell l_spell(nullptr);
			luabind::object l_spell_o = luabind::object(L, l_spell);
			l_spell_o.push(L);
		}
		lua_setfield(L, -2, "self");
		
		auto arg_function = SpellArgumentDispatch[evt];
		arg_function(this, L, npc, client, spell_id, extra_data, extra_pointers);
		
		quest_manager.StartQuest(npc, client, nullptr);
		if(lua_pcall(L, 1, 1, 0)) {
			std::string error = lua_tostring(L, -1);
			AddError(error);
			quest_manager.EndQuest();
			return 0;
		}
		quest_manager.EndQuest();
		
		if(lua_isnumber(L, -1)) {
			int ret = static_cast<int>(lua_tointeger(L, -1));
			lua_pop(L, npop);
			return ret;
		}
		
		lua_pop(L, npop);
	} catch(std::exception &ex) {
		std::string error = "Lua Exception: ";
		error += std::string(ex.what());
		AddError(error);

		//Restore our stack to the best of our ability
		int end = lua_gettop(L);
		int n = end - start;
		if(n > 0) {
			lua_pop(L, n);
		}
	}

	return 0;
}

int LuaParser::EventEncounter(QuestEventID evt, std::string encounter_name, uint32 extra_data, std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	std::string package_name = "encounter_" + encounter_name;

	if(!EncounterHasQuestSub(encounter_name, evt)) {
		return 0;
	}
	
	return _EventEncounter(package_name, evt, encounter_name, extra_data, extra_pointers);
}

int LuaParser::_EventEncounter(std::string package_name, QuestEventID evt, std::string encounter_name, uint32 extra_data,
							   std::vector<void*> *extra_pointers) {
	const char *sub_name = LuaEvents[evt];
	
	int start = lua_gettop(L);

	try {
		lua_getfield(L, LUA_REGISTRYINDEX, package_name.c_str());
		lua_getfield(L, -1, sub_name);
	
		lua_createtable(L, 0, 0);
		lua_pushstring(L, encounter_name.c_str());
		lua_setfield(L, -2, "name");

		quest_manager.StartQuest(nullptr, nullptr, nullptr);
		if(lua_pcall(L, 1, 1, 0)) {
			std::string error = lua_tostring(L, -1);
			AddError(error);
			quest_manager.EndQuest();
			return 0;
		}
		quest_manager.EndQuest();
		
		if(lua_isnumber(L, -1)) {
			int ret = static_cast<int>(lua_tointeger(L, -1));
			lua_pop(L, 2);
			return ret;
		}
		
		lua_pop(L, 2);
	} catch(std::exception &ex) {
		std::string error = "Lua Exception: ";
		error += std::string(ex.what());
		AddError(error);

		//Restore our stack to the best of our ability
		int end = lua_gettop(L);
		int n = end - start;
		if(n > 0) {
			lua_pop(L, n);
		}
	}

	return 0;
}

bool LuaParser::HasQuestSub(uint32 npc_id, QuestEventID evt) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return false;
	}

	std::string package_name = "npc_" + std::to_string(static_cast<long long>(npc_id));

	const char *subname = LuaEvents[evt];
	return HasFunction(subname, package_name);
}

bool LuaParser::HasGlobalQuestSub(QuestEventID evt) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return false;
	}

	const char *subname = LuaEvents[evt];
	return HasFunction(subname, "global_npc");
}

bool LuaParser::PlayerHasQuestSub(QuestEventID evt) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return false;
	}

	const char *subname = LuaEvents[evt];
	return HasFunction(subname, "player");
}

bool LuaParser::GlobalPlayerHasQuestSub(QuestEventID evt) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return false;
	}

	const char *subname = LuaEvents[evt];
	return HasFunction(subname, "global_player");
}

bool LuaParser::SpellHasQuestSub(uint32 spell_id, QuestEventID evt) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return false;
	}

	std::string package_name = "spell_" + std::to_string(static_cast<long long>(spell_id));

	const char *subname = LuaEvents[evt];
	return HasFunction(subname, package_name);
}

bool LuaParser::ItemHasQuestSub(ItemInst *itm, QuestEventID evt) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return false;
	}

	std::string package_name = "item_";
	package_name += std::to_string(static_cast<long long>(itm->GetID()));

	const char *subname = LuaEvents[evt];
	return HasFunction(subname, package_name);
}

bool LuaParser::EncounterHasQuestSub(std::string encounter_name, QuestEventID evt) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return false;
	}

	std::string package_name = "encounter_" + encounter_name;

	const char *subname = LuaEvents[evt];
	return HasFunction(subname, package_name);
}

void LuaParser::LoadNPCScript(std::string filename, int npc_id) {
	std::string package_name = "npc_" + std::to_string(static_cast<long long>(npc_id));

	LoadScript(filename, package_name);
}

void LuaParser::LoadGlobalNPCScript(std::string filename) {
	LoadScript(filename, "global_npc");
}

void LuaParser::LoadPlayerScript(std::string filename) {
	LoadScript(filename, "player");
}

void LuaParser::LoadGlobalPlayerScript(std::string filename) {
	LoadScript(filename, "global_player");
}

void LuaParser::LoadItemScript(std::string filename, ItemInst *item) {
	std::string package_name = "item_";
	package_name += std::to_string(static_cast<long long>(item->GetID()));

	LoadScript(filename, package_name);
}

void LuaParser::LoadSpellScript(std::string filename, uint32 spell_id) {
	std::string package_name = "spell_" + std::to_string(static_cast<long long>(spell_id));

	LoadScript(filename, package_name);
}

void LuaParser::LoadEncounterScript(std::string filename, std::string encounter_name) {
	std::string package_name = "encounter_" + encounter_name;

	LoadScript(filename, package_name);
}

void LuaParser::AddVar(std::string name, std::string val) {
	vars_[name] = val;
}

std::string LuaParser::GetVar(std::string name) {
	auto iter = vars_.find(name);
	if(iter != vars_.end()) {
		return iter->second;
	}

	return std::string();
}

void LuaParser::Init() {
	ReloadQuests();
}

void LuaParser::ReloadQuests() {
	loaded_.clear();
	errors_.clear();
	lua_encounter_events_registered.clear();

	if(L) {
		lua_close(L);
	}

	L = luaL_newstate();
	luaL_openlibs(L);

	if(luaopen_bit(L) != 1) {
		std::string error = lua_tostring(L, -1);
		AddError(error);
	}

	if(luaL_dostring(L, "math.randomseed(os.time())")) {
		std::string error = lua_tostring(L, -1);
		AddError(error);
	}

#ifdef SANITIZE_LUA_LIBS
	//io
	lua_pushnil(L);
	lua_setglobal(L, "io");

	//some os/debug are okay some are not
	lua_getglobal(L, "os");
	lua_pushnil(L);
	lua_setfield(L, -2, "exit");
	lua_pushnil(L);
	lua_setfield(L, -2, "execute");
	lua_pushnil(L);
	lua_setfield(L, -2, "getenv");
	lua_pushnil(L);
	lua_setfield(L, -2, "remove");
	lua_pushnil(L);
	lua_setfield(L, -2, "rename");
	lua_pushnil(L);
	lua_setfield(L, -2, "setlocale");
	lua_pushnil(L);
	lua_setfield(L, -2, "tmpname");
	lua_pop(L, 1);

	lua_pushnil(L);
	lua_setglobal(L, "collectgarbage");

	lua_pushnil(L);
	lua_setglobal(L, "loadfile");

#endif

	lua_getglobal(L, "package");
	lua_getfield(L, -1, "path");
	std::string module_path = lua_tostring(L,-1);
	module_path += ";./lua_modules/?.lua";
	lua_pop(L, 1);
	lua_pushstring(L, module_path.c_str());
	lua_setfield(L, -2, "path");
	lua_pop(L, 1);

	MapFunctions(L);

	//load init
	std::string path = "quests/";
	path += QUEST_GLOBAL_DIRECTORY;
	path += "/script_init.lua";

	FILE *f = fopen(path.c_str(), "r");
	if(f) {
		fclose(f);
	
		if(luaL_dofile(L, "quests/global/script_init.lua")) {
			std::string error = lua_tostring(L, -1);
			AddError(error);
		}
	}
	
	//zone init - always loads after global
	if(zone) {
		std::string zone_script = "quests/";
		zone_script += zone->GetShortName();
		zone_script += "/script_init.lua";
		f = fopen(zone_script.c_str(), "r");
		if(f) {
			fclose(f);
		
			if(luaL_dofile(L, zone_script.c_str())) {
				std::string error = lua_tostring(L, -1);
				AddError(error);
			}
		}
	}
}

void LuaParser::LoadScript(std::string filename, std::string package_name) {
	auto iter = loaded_.find(package_name);
	if(iter != loaded_.end()) {
		return;
	}
	
	if(luaL_loadfile(L, filename.c_str())) {
		std::string error = lua_tostring(L, -1);
		AddError(error);
		lua_pop(L, 1);
		return;
	}

	//This makes an env table named: package_name
	//And makes it so we can see the global table _G from it
	//Then sets it so this script is called from that table as an env

	lua_createtable(L, 0, 0); // anon table
	lua_getglobal(L, "_G"); // get _G
	lua_setfield(L, -2, "__index"); //anon table.__index = _G

	lua_pushvalue(L, -1); //copy table to top of stack
	lua_setmetatable(L, -2); //setmetatable(anon_table, copied table)

	lua_pushvalue(L, -1); //put the table we made into the registry
	lua_setfield(L, LUA_REGISTRYINDEX, package_name.c_str());

	lua_setfenv(L, -2); //set the env to the table we made

	if(lua_pcall(L, 0, 0, 0)) {
		std::string error = lua_tostring(L, -1);
		AddError(error);
		lua_pop(L, 1);
		return;
	}

	loaded_[package_name] = true;
}

bool LuaParser::HasFunction(std::string subname, std::string package_name) {
	std::transform(subname.begin(), subname.end(), subname.begin(), ::tolower);

	auto iter = loaded_.find(package_name);
	if(iter == loaded_.end()) {
		return false;
	}

	lua_getfield(L, LUA_REGISTRYINDEX, package_name.c_str());
	lua_getfield(L, -1, subname.c_str());

	if(lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return true;
	}

	lua_pop(L, 2);
	return false;
}

void LuaParser::MapFunctions(lua_State *L) {

	try {
		luabind::open(L);

		luabind::module(L)
		[
			lua_register_general(),
			lua_register_events(),
			lua_register_faction(),
			lua_register_slot(),
			lua_register_material(),
			lua_register_client_version(),
			lua_register_appearance(),
			lua_register_entity(),
			lua_register_mob(),
			lua_register_special_abilities(),
			lua_register_npc(),
			lua_register_client(),
			lua_register_inventory(),
			lua_register_inventory_where(),
			lua_register_iteminst(),
			lua_register_item(),
			lua_register_spell(),
			lua_register_spawn(),
			lua_register_hate_entry(),
			lua_register_hate_list(),
			lua_register_entity_list(),
			lua_register_mob_list(),
			lua_register_client_list(),
			lua_register_npc_list(),
			lua_register_corpse_list(),
			lua_register_object_list(),
			lua_register_door_list(),
			lua_register_spawn_list(),
//			lua_register_group(),
//			lua_register_raid(),
			lua_register_corpse(),
			lua_register_door(),
			lua_register_object(),
			lua_register_packet(),
			lua_register_packet_opcodes()
		];
	
	} catch(std::exception &ex) {
		std::string error = ex.what();
		AddError(error);
	}
}

int LuaParser::DispatchEventNPC(QuestEventID evt, NPC* npc, Mob *init, std::string data, uint32 extra_data,
								 std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	if(!npc)
		return 0;

	std::string package_name = "npc_" + std::to_string(static_cast<long long>(npc->GetNPCTypeID()));
    int ret = 0;

	auto iter = lua_encounter_events_registered.find(package_name);
	if(iter != lua_encounter_events_registered.end()) {
		auto riter = iter->second.begin();
		while(riter != iter->second.end()) {
			if(riter->event_id == evt) {
				std::string package_name = "encounter_" + riter->encounter_name;
				int i = _EventNPC(package_name, evt, npc, init, data, extra_data, extra_pointers, &riter->lua_reference);
                if(i != 0)
                    ret = i;
			}
			++riter;
		}
	}

	iter = lua_encounter_events_registered.find("npc_-1");
	if(iter == lua_encounter_events_registered.end()) {
		return ret;
	}

	auto riter = iter->second.begin();
	while(riter != iter->second.end()) {
		if(riter->event_id == evt) {
			std::string package_name = "encounter_" + riter->encounter_name;
			int i = _EventNPC(package_name, evt, npc, init, data, extra_data, extra_pointers, &riter->lua_reference);
            if(i != 0)
                ret = i;
		}
		++riter;
	}

    return ret;
}

int LuaParser::DispatchEventPlayer(QuestEventID evt, Client *client, std::string data, uint32 extra_data,
									std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	std::string package_name = "player";

	auto iter = lua_encounter_events_registered.find(package_name);
	if(iter == lua_encounter_events_registered.end()) {
		return 0;
	}

    int ret = 0;
	auto riter = iter->second.begin();
	while(riter != iter->second.end()) {
		if(riter->event_id == evt) {
			std::string package_name = "encounter_" + riter->encounter_name;
			int i = _EventPlayer(package_name, evt, client, data, extra_data, extra_pointers, &riter->lua_reference);
            if(i != 0)
                ret = i;
		}
		++riter;
	}

    return ret;
}

int LuaParser::DispatchEventItem(QuestEventID evt, Client *client, ItemInst *item, Mob *mob, std::string data, uint32 extra_data,
								  std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	if(!item)
		return 0;
	
	std::string package_name = "item_";
	package_name += std::to_string(static_cast<long long>(item->GetID()));
    int ret = 0;
	
	auto iter = lua_encounter_events_registered.find(package_name);
	if(iter != lua_encounter_events_registered.end()) {
		auto riter = iter->second.begin();
		while(riter != iter->second.end()) {
			if(riter->event_id == evt) {
				std::string package_name = "encounter_" + riter->encounter_name;
				int i = _EventItem(package_name, evt, client, item, mob, data, extra_data, extra_pointers, &riter->lua_reference);
                if(i != 0)
                    ret = i;
			}
			++riter;
		}
	}

	iter = lua_encounter_events_registered.find("item_-1");
	if(iter == lua_encounter_events_registered.end()) {
		return ret;
	}
	
	auto riter = iter->second.begin();
	while(riter != iter->second.end()) {
		if(riter->event_id == evt) {
			std::string package_name = "encounter_" + riter->encounter_name;
			int i = _EventItem(package_name, evt, client, item, mob, data, extra_data, extra_pointers, &riter->lua_reference);
            if(i != 0)
                ret = i;
		}
		++riter;
	}
    return ret;
}

int LuaParser::DispatchEventSpell(QuestEventID evt, NPC* npc, Client *client, uint32 spell_id, uint32 extra_data,
								   std::vector<void*> *extra_pointers) {
	evt = ConvertLuaEvent(evt);
	if(evt >= _LargestEventID) {
		return 0;
	}

	std::string package_name = "spell_" + std::to_string(static_cast<long long>(spell_id));

    int ret = 0;
	auto iter = lua_encounter_events_registered.find(package_name);
	if(iter != lua_encounter_events_registered.end()) {
	    auto riter = iter->second.begin();
		while(riter != iter->second.end()) {
			if(riter->event_id == evt) {
				std::string package_name = "encounter_" + riter->encounter_name;
				int i = _EventSpell(package_name, evt, npc, client, spell_id, extra_data, extra_pointers, &riter->lua_reference);
                if(i != 0) {
                    ret = i;
                }
			}
			++riter;
		}
	}

	iter = lua_encounter_events_registered.find("spell_-1");
	if(iter == lua_encounter_events_registered.end()) {
		return ret;
	}

	auto riter = iter->second.begin();
	while(riter != iter->second.end()) {
		if(riter->event_id == evt) {
			std::string package_name = "encounter_" + riter->encounter_name;
			int i = _EventSpell(package_name, evt, npc, client, spell_id, extra_data, extra_pointers, &riter->lua_reference);
            if(i != 0)
                ret = i;
		}
		++riter;
	}
    return ret;
}

QuestEventID LuaParser::ConvertLuaEvent(QuestEventID evt) {
	switch(evt) {
	case EVENT_SLAY:
	case EVENT_NPC_SLAY:
		return EVENT_SLAY;
		break;
	case EVENT_SPELL_EFFECT_CLIENT:
	case EVENT_SPELL_EFFECT_NPC:
		return EVENT_SPELL_EFFECT_CLIENT;
		break;
	case EVENT_SPELL_BUFF_TIC_CLIENT:
	case EVENT_SPELL_BUFF_TIC_NPC:
		return EVENT_SPELL_BUFF_TIC_CLIENT;
		break;
	case EVENT_AGGRO:
	case EVENT_ATTACK:
		return _LargestEventID;
		break;
	default:
		return evt;
	}
}

#endif
