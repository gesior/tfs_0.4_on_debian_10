////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////

#ifndef __CONFIG_MANAGER__
#define __CONFIG_MANAGER__
#include "luascript.h"

class ConfigManager
{
	public:
		ConfigManager();
		virtual ~ConfigManager() {}

		enum string_config_t
		{
			DUMMY_STR = 0,
			CONFIG_FILE,
			MAP_NAME,
			HOUSE_RENT_PERIOD,
			LOGIN_MSG,
			FIRST_MSG,
			SERVER_NAME,
			OWNER_NAME,
			OWNER_EMAIL,
			URL,
			LOCATION,
			IP,
			MOTD,
			WORLD_TYPE,
			SQL_HOST,
			SQL_USER,
			SQL_PASS,
			SQL_DB,
			DEFAULT_PRIORITY,
			#ifdef MULTI_SQL_DRIVERS
			SQL_TYPE,
			#endif
			SQL_FILE,
			ENCRYPTION_TYPE,
			ENCRYPTION_KEY,
			MAP_AUTHOR,
			RUNFILE,
			OUTPUT_LOG,
			DATA_DIRECTORY,
			LOGS_DIRECTORY,
			PREFIX_CHANNEL_LOGS,
			CORES_USED,
			MAILBOX_DISABLED_TOWNS,
			MANAGER_PASSWORD,
			ADMIN_PASSWORD,
			ADMIN_ENCRYPTION,
			ADMIN_ENCRYPTION_DATA,
			LAST_STRING_CONFIG /* this must be the last one */
		};

		enum number_config_t
		{
			LOGIN_TRIES = 0,
			RETRY_TIMEOUT,
			LOGIN_TIMEOUT,
			LOGIN_PORT,
			GAME_PORT,
			ADMIN_PORT,
			STATUS_PORT,
			MANAGER_PORT,
			SQL_PORT,
			SQL_KEEPALIVE,
			MAX_PLAYERS,
			PZ_LOCKED,
			HUNTING_DURATION,
			DEFAULT_DESPAWNRANGE,
			DEFAULT_DESPAWNRADIUS,
			RATE_SPAWN,
			SPAWNPOS_X,
			SPAWNPOS_Y,
			SPAWNPOS_Z,
			SPAWNTOWN_ID,
			ALLOW_CLONES,
			GLOBALSAVE_H,
			GLOBALSAVE_M,
			START_LEVEL,
			START_MAGICLEVEL,
			HOUSE_PRICE,
			HIGHSCORES_TOP,
			MAX_MESSAGEBUFFER,
			HIGHSCORES_UPDATETIME,
			ACTIONS_DELAY_INTERVAL,
			EX_ACTIONS_DELAY_INTERVAL,
			CRITICAL_HIT_CHANCE,
			PROTECTION_LEVEL,
			ENCRYPTION,
			STATUSQUERY_TIMEOUT,
			LEVEL_TO_FORM_GUILD,
			MIN_GUILDNAME,
			MAX_GUILDNAME,
			LEVEL_TO_BUY_HOUSE,
			HOUSES_PER_ACCOUNT,
			WHITE_SKULL_TIME,
			RED_SKULL_LENGTH,
			BLACK_SKULL_LENGTH,
			MAX_VIOLATIONCOMMENT_SIZE,
			NOTATIONS_TO_BAN,
			WARNINGS_TO_FINALBAN,
			WARNINGS_TO_DELETION,
			BAN_LENGTH,
			KILLS_BAN_LENGTH,
			FINALBAN_LENGTH,
			IPBANISHMENT_LENGTH,
			MAX_PLAYER_SUMMONS,
			FIELD_OWNERSHIP,
			WORLD_ID,
			EXTRA_PARTY_PERCENT,
			EXTRA_PARTY_LIMIT,
			MYSQL_READ_TIMEOUT,
			MYSQL_WRITE_TIMEOUT,
			PARTY_RADIUS_X,
			PARTY_RADIUS_Y,
			PARTY_RADIUS_Z,
			LOGIN_PROTECTION,
			PLAYER_DEEPNESS,
			STAIRHOP_DELAY,
			RATE_STAMINA_LOSS,
			STAMINA_LIMIT_TOP,
			STAMINA_LIMIT_BOTTOM,
			BLESS_REDUCTION_BASE,
			BLESS_REDUCTION_DECREMENT,
			BLESS_REDUCTION,
			NICE_LEVEL,
			EXPERIENCE_COLOR,
			GUILD_PREMIUM_DAYS,
			PUSH_CREATURE_DELAY,
			DEATH_CONTAINER,
			MAXIMUM_DOOR_LEVEL,
			DEATH_ASSISTS,
			RED_DAILY_LIMIT,
			RED_WEEKLY_LIMIT,
			RED_MONTHLY_LIMIT,
			BLACK_DAILY_LIMIT,
			BLACK_WEEKLY_LIMIT,
			BLACK_MONTHLY_LIMIT,
			BAN_DAILY_LIMIT,
			BAN_WEEKLY_LIMIT,
			BAN_MONTHLY_LIMIT,
			BLACK_SKULL_DEATH_HEALTH,
			BLACK_SKULL_DEATH_MANA,
			DEATHLIST_REQUIRED_TIME,
			EXPERIENCE_SHARE_ACTIVITY,
			TILE_LIMIT,
			PROTECTION_TILE_LIMIT,
			HOUSE_TILE_LIMIT,
			SQUARE_COLOR,
			LOOT_MESSAGE,
			LOOT_MESSAGE_TYPE,
			NAME_REPORT_TYPE,
			HOUSE_CLEAN_OLD,
			MANAGER_CONNECTIONS_LIMIT,
			ADMIN_CONNECTIONS_LIMIT,
			VIPLIST_DEFAULT_LIMIT,
			VIPLIST_DEFAULT_PREMIUM_LIMIT,
			STAMINA_DESTROY_LOOT,
			FIST_BASE_ATTACK,
			LAST_NUMBER_CONFIG /* this must be the last one */
		};

		enum double_config_t
		{
			RATE_EXPERIENCE,
			RATE_SKILL,
			RATE_MAGIC,
			RATE_LOOT,
			PARTY_DIFFERENCE,
			CRITICAL_HIT_MUL,
			RATE_STAMINA_GAIN,
			RATE_STAMINA_THRESHOLD,
			RATE_STAMINA_ABOVE,
			RATE_STAMINA_UNDER,
			EFP_MIN_THRESHOLD,
			EFP_MAX_THRESHOLD,
			RATE_PVP_EXPERIENCE,
			RATE_MONSTER_HEALTH,
			RATE_MONSTER_MANA,
			RATE_MONSTER_ATTACK,
			RATE_MONSTER_DEFENSE,
			FORMULA_LEVEL,
			FORMULA_MAGIC,
			LAST_DOUBLE_CONFIG /* this must be the last one */
		};

		enum bool_config_t
		{
			GLOBALSAVE_ENABLED = 0,
			SCRIPT_SYSTEM,
			START_CLOSED,
			START_CHOOSEVOC,
			ON_OR_OFF_CHARLIST,
			ONE_PLAYER_ON_ACCOUNT,
			REMOVE_WEAPON_AMMO,
			REMOVE_WEAPON_CHARGES,
			REMOVE_RUNE_CHARGES,
			RANDOMIZE_TILES,
			SHUTDOWN_AT_GLOBALSAVE,
			CLEAN_MAP_AT_GLOBALSAVE,
			FREE_PREMIUM,
			GENERATE_ACCOUNT_NUMBER,
			BANK_SYSTEM,
			INIT_PREMIUM_UPDATE,
			TELEPORT_SUMMONS,
			TELEPORT_PLAYER_SUMMONS,
			PVP_TILE_IGNORE_PROTECTION,
			DISPLAY_CRITICAL_HIT,
			ADVANCING_SKILL_LEVEL,
			CLEAN_PROTECTED_ZONES,
			SPELL_NAME_INSTEAD_WORDS,
			EMOTE_SPELLS,
			REPLACE_KICK_ON_LOGIN,
			PREMIUM_FOR_PROMOTION,
			SHOW_HEALING_DAMAGE,
			BROADCAST_BANISHMENTS,
			SAVE_GLOBAL_STORAGE,
			INGAME_GUILD_MANAGEMENT,
			HOUSE_BUY_AND_SELL,
			HOUSE_NEED_PREMIUM,
			HOUSE_RENTASPRICE,
			HOUSE_PRICEASRENT,
			ACCOUNT_MANAGER,
			NAMELOCK_MANAGER,
			ALLOW_CHANGEOUTFIT,
			CANNOT_ATTACK_SAME_LOOKFEET,
			AIMBOT_HOTKEY_ENABLED,
			FORCE_CLOSE_SLOW_CONNECTION,
			EXPERIENCE_STAGES,
			BLESSINGS,
			BLESSING_ONLY_PREMIUM,
			BED_REQUIRE_PREMIUM,
			ALLOW_CHANGECOLORS,
			LOGIN_ONLY_LOGINSERVER,
			STOP_ATTACK_AT_EXIT,
			DISABLE_OUTFITS_PRIVILEGED,
			OPTIMIZE_DATABASE,
			STORE_TRASH,
			HOUSE_STORAGE,
			TRUNCATE_LOG,
			TRACER_BOX,
			STORE_DIRECTION,
			DISPLAY_LOGGING,
			STAMINA_BONUS_PREMIUM,
			BAN_UNKNOWN_BYTES,
			ALLOW_CHANGEADDONS,
			GHOST_INVISIBLE_EFFECT,
			SHOW_HEALING_DAMAGE_MONSTER,
			CHECK_CORPSE_OWNER,
			BUFFER_SPELL_FAILURE,
			CONFIRM_OUTDATED_VERSION,
			PREMIUM_SKIP_WAIT,
			GUILD_HALLS,
			DEATH_LIST,
			GHOST_SPELL_EFFECTS,
			PVPZONE_ADDMANASPENT,
			USE_BLACK_SKULL,
			USE_FRAG_HANDLER,
			ALLOW_FIGHTBACK,
			VIPLIST_PER_PLAYER,
			MANAGER_LOCALHOST_ONLY,
			MANAGER_LOGS,
			ADMIN_LOGS,
			ADMIN_LOCALHOST_ONLY,
			ADMIN_REQUIRE_LOGIN,
			ADDONS_PREMIUM,
#ifdef __WAR_SYSTEM__
			OPTIONAL_WAR_ATTACK_ALLY,
#endif
			LAST_BOOL_CONFIG /* this must be the last one */
		};

		bool load();
		bool reload();
		void startup() {m_startup = false;}

		bool isRunning() const {return !m_startup;}
		bool isLoaded() const {return m_loaded;}

		const std::string& getString(uint32_t _what) const;
		bool getBool(uint32_t _what) const;
		int32_t getNumber(uint32_t _what) const;
		double getDouble(uint32_t _what) const;

		bool setString(uint32_t _what, const std::string& _value);
		bool setNumber(uint32_t _what, int32_t _value);
		bool setBool(uint32_t _what, bool _value);

		void getValue(const std::string& key, lua_State* _L) {LuaInterface::getValue(key, L, _L);}

	private:
		static void moveValue(lua_State* fromL, lua_State* toL);

		std::string getGlobalString(const std::string& _identifier, const std::string& _default = "")
		{
			return LuaInterface::getGlobalString(L, _identifier, _default);
		}
		bool getGlobalBool(const std::string& _identifier, bool _default = false)
		{
			return LuaInterface::getGlobalBool(L, _identifier, _default);
		}
		int32_t getGlobalNumber(const std::string& _identifier, const int32_t _default = 0)
		{
			return LuaInterface::getGlobalNumber(L, _identifier, _default);
		}
		double getGlobalDouble(const std::string& _identifier, const double _default = 0)
		{
			return LuaInterface::getGlobalDouble(L, _identifier, _default);
		}

		bool m_loaded, m_startup;
		lua_State* L;

		std::string m_confString[LAST_STRING_CONFIG];
		bool m_confBool[LAST_BOOL_CONFIG];
		int32_t m_confNumber[LAST_NUMBER_CONFIG];
		double m_confDouble[LAST_DOUBLE_CONFIG];
};
#endif
