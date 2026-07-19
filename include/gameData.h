#pragma once
#include <map>
#include <string>
#include <vector>
#include <optional>
#include <raylib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ActiveBuff {
    std::string stat;
    float delta;
    int duration;
    std::string label = "";
};

struct Stats {
    int ATK, HP;
    float DEF, CRIT_RATE, CRIT_DMG;
    int maxHP = 0;
    std::vector<ActiveBuff> activeBuffs;
};

struct SkillModifiers{
    std::string type;
    std::map<std::string, json> additionalParams;
};

struct Skill {
    std::string title, desc, id, type, target;
    int cooldown = 0;
    int baseCooldown = 0;
    Texture2D *icon = nullptr;
    std::vector<SkillModifiers> modifiers;
};

struct Character {
    std::string name, portrait_id;
    bool isEnemy = false;
    Stats stats;
    static constexpr int SKILLS = 4;
    std::vector<Skill> skills;
    Texture2D *portrait = nullptr;
};



enum class ItemCategory { ALL, QUEST, CRAFTING, POTIONS, MISC };

struct InventoryItem {
    std::string id;
    std::string name;
    std::string description;
    ItemCategory category;
    std::string icon_id;
    Texture2D *icon = nullptr;
};

struct InventorySlot{
    const InventoryItem *item;
    int quantity = 0;
};

struct Rewards{
    const InventoryItem *item;
    int amount;
};

struct Inventory {
    static constexpr int rows = 10;
    static constexpr int cols = 20;
    std::map<std::string, InventoryItem>items;
    std::map<const InventoryItem*, int>slots;

    const InventoryItem* GetItem(std::string& item_id){
        auto it = items.find(item_id);
        return (it != items.end()) ? &(it->second) : nullptr;
    }

    void AddToInventory(const std::vector<Rewards>& rewards){
        for(auto &reward: rewards){
            if(slots.size() < 1){
                slots.insert(std::make_pair(reward.item, reward.amount));
            }
            else{
                auto it = slots.find(reward.item);
                if(it != slots.end()) it->second += reward.amount;
                else slots.insert(std::make_pair(reward.item, reward.amount));
            }
        }
    }
};

enum class EventType{
    TALK_TO_NPC,
    COLLECT_ITEM,
    KILL,
    COMPLETE_QUEST
};
struct Events{
    EventType event;
    std::string target_id;
    int amount = 1;
};

struct Quest {
    std::string questTitle, questDesc, questTask, id;
    bool locked = true, complete = false, rewardCollected = false;
    int progress = 0, rewardCount;

    std::vector<Rewards> rewards;
    Events unlockEvent, endEvent;
};

struct Resources {
    std::map<std::string, Texture2D> texture;
    std::map<std::string, Sound> sfx;
    std::map<std::string, Music> music;

    void Load();
    ~Resources();

    Texture2D *GetTexture(const std::string &id);
    Sound *GetSfx(const std::string &id);
    Music *GetMusic(const std::string &id);
};

struct BattleReport {
    bool victory = false;
    std::vector<std::string> defeatedEnemyNames;
};

struct GameData {
    Resources res;
    std::vector<Character> characters;
    std::vector<Quest> quests;
    std::vector<Quest> dailyTasks;
    Inventory inventory;
    Quest* trackingQuest = nullptr, *newQuest = nullptr;
    std::array<Character*, 3> inBattleCharacters;
    bool fighting = false;
    int playableCharacterCount = 0;
    Vector2 preBattleSpawnPosition = {-1, -1};
    Vector2 playerPosition = {0.0f, 0.0f};
    std::string currentMap = "data/map/tilemap/tilemap.tmj";

    void LoadAll();
    void SaveGame() const;
    void LoadSave();
    void DeleteSave();
    Character* GetCharacter(const std::string &id);
    void QuestManager(EventType event, std::string target, int amount = 1){
        for(auto &quest : quests){
            if(quest.unlockEvent.event == event && quest.unlockEvent.target_id == target && quest.locked){
                newQuest = &quest;
                quest.locked = false;
                if(trackingQuest == nullptr) trackingQuest = &quest;
                PlaySound(*(res.GetSfx("quest_new")));
            }
            else if(quest.endEvent.event == event && quest.endEvent.target_id == target && !quest.locked){
                if(quest.complete) continue;
                quest.progress += amount;
                if(quest.progress >= quest.endEvent.amount){
                    quest.complete = true;
                    trackingQuest = &quest;
                    PlaySound(*(res.GetSfx("quest_complete")));
                }
            }
        }
    }

    /*
    Post-battle integration entry point. Decouples the combat engine from the
    quest system: the battle only needs to produce a BattleReport, and this
    resolver translates defeated enemies into generic QuestManager KILL events.
    Future battle-linked objectives (multiple enemies, partial credit, etc.) are
    handled here without touching the combat engine.
    */
    void OnBattleResolved(const BattleReport& report){
        if(!report.victory) return;
        for(const auto& enemyName : report.defeatedEnemyNames){
            QuestManager(EventType::KILL, enemyName, 1);
        }
    }
};

extern GameData gameData;

void LoadCharacters(GameData &data);
void LoadQuests(GameData &data);
void LoadDailyTasks(GameData &data);
void LoadInventory(GameData &data);
