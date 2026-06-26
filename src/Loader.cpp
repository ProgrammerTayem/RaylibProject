#include "Loader.h"
#include "gameScene.h"
#include "gameData.h"
#include <fstream>
#include <stdexcept>
#include <ctime>

GameData gameData;
static void WriteErrorLog(const std::string& message);

Texture2D *Resources::GetTexture(const std::string &id){
    auto it = texture.find(id);
    if(it == texture.end()){
        WriteErrorLog("ERROR: Texture id not found: " + id);
        return nullptr;
    }
    return &it->second;
}
Sound *Resources::GetSfx(const std::string &id){
    auto it = sfx.find(id);
    if(it == sfx.end()){
        WriteErrorLog("ERROR: Sfx id not found: " + id);
        return nullptr;
    }
    return &it->second;
}
Music *Resources::GetMusic(const std::string &id){
    auto it = music.find(id);
    if(it == music.end()){
        WriteErrorLog("ERROR: Music id not found: " + id);
        return nullptr;
    }
    return &it->second;
}

static void WriteErrorLog(const std::string& message) {
    std::ofstream logFile("ErrorLog", std::ios::app);
    if(logFile.is_open()){
        std::time_t now = std::time(nullptr);
        char timestamp[64];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        logFile << "[" << timestamp << "] " << message << '\n';
    }
}

void LoadTileMap(TilemapData &data, const std::string& tilemap, World& world, Resources& res){
    std::ifstream file(tilemap);
    if(!file.is_open()){
        WriteErrorLog(std::string("ERROR: Could not open tilemap file: ") + tilemap);
        throw std::runtime_error("Could not open tilemap file: " + tilemap);
    }
    
    json j;
    try {
        file >> j;
    } catch(const json::exception& e) {
        WriteErrorLog(std::string("ERROR: JSON parsing error: ") + e.what());
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }

    data.width = j.value("width", 0);
    data.height = j.value("height", 0);
    
    if(data.width <= 0 || data.height <= 0){
        std::string message = "ERROR: Invalid tilemap dimensions: " +
                              std::to_string(data.width) + "x" +
                              std::to_string(data.height);
        WriteErrorLog(message);
        throw std::runtime_error(message);
    }
    
    if(!j.contains("layers") || j["layers"].empty()){
        WriteErrorLog("ERROR: Tilemap has no layers");
        throw std::runtime_error("Tilemap has no layers");
    }
    
    auto& firstLayer = j["layers"][0];
    if(!firstLayer.contains("data")){
        WriteErrorLog("ERROR: Layer has no tile data");
        throw std::runtime_error("Layer has no tile data");
    }
    
    try {
        data.tileData = firstLayer["data"].get<std::vector<int>>();
    } catch (const json::exception& e) {
        WriteErrorLog(std::string("ERROR: Error extracting tile data: ") + e.what());
        throw std::runtime_error("Error extracting tile data: " + std::string(e.what()));
    }
    
    if(data.tileData.size() != static_cast<size_t>(data.width * data.height)){
        std::string message = "ERROR: Tile data size mismatch: expected " +
                              std::to_string(data.width * data.height) +
                              " tiles, got " +
                              std::to_string(data.tileData.size());
        WriteErrorLog(message);
        throw std::runtime_error(message);
    }

    auto& gameObjLayer = j["layers"][1];
    if(!gameObjLayer.contains("objects")){
        WriteErrorLog("WARNING: Layer has no GameObject data");
        throw std::runtime_error("Layer has no GameObject data");
    }

    for(auto &data : gameObjLayer["objects"]){
        if(!data.contains("type")){
            WriteErrorLog("WARNING: Object has no GameObject data");
            throw std::runtime_error("Object has no GameObject data");
        }
        std::string type, name;
        std::string dialoguePath;
        int x, y;
        try {
            type = data["type"].get<std::string>();
            name = data["name"].get<std::string>();
            x = data["x"].get<int>();
            y = data["y"].get<int>();
            dialoguePath = data["properties"][0]["value"].get<std::string>();
        } catch (const json::exception& e) {
            WriteErrorLog(std::string("ERROR: Error extracting GameObject data: ") + e.what());
            throw std::runtime_error("Error extracting GameObject data: " + std::string(e.what()));
        }

        if(type == "NPC"){
            std::ifstream dialogue(dialoguePath);
            if(!dialogue.is_open()){
                WriteErrorLog(std::string("ERROR: Could not open dialogue file: ") + dialoguePath);
                throw std::runtime_error("Could not open dialogue file: " + dialoguePath);
            }
            std::vector<std::string> dialogueLines;
            json k;

            try {
                dialogue >> k;
            } catch(const json::exception& e) {
                WriteErrorLog(std::string("ERROR: JSON parsing error: ") + e.what());
                throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
            }

            dialogueLines = k["lines"].get<std::vector<std::string>>();
            k.clear();

            NPC n = n.MakeNPC(name, Vector2(x, y), dialogueLines);
            auto npc = std::make_unique<NPC>(n);
            world.entities.push_back(std::move(npc));
        }

        if(type == "Enemy"){
            auto enm = std::make_unique<Enemy>(res, (Vector2){x, y}, name);
            world.entities.push_back(std::move(enm));
        }
    }
    file.close();
    j.clear();
}

void LoadTileset(TilemapData &data, const std::string& tileset){
    std::ifstream file(tileset);
    if(!file.is_open()){
        WriteErrorLog(std::string("ERROR: Could not open tileset file: ") + tileset);
        throw std::runtime_error("Could not open tileset file: " + tileset);
    }

    json j;

    try {
        file >> j;
    } catch (const json::exception& e) {
        WriteErrorLog(std::string("ERROR: JSON parsing error: ") + e.what());
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }

    for(const auto &tile : j["tiles"]){
        if(!tile.contains("id") || tile.empty()){
            WriteErrorLog("ERROR: Tiles has no tile data");
            throw std::runtime_error("Tiles has no tile data");
        }
        int id = tile["id"].get<int>();

        if(tile.contains("properties")){
            for(const auto& prop : tile["properties"]){
                if(!prop.contains("value") || prop.empty()){
                    WriteErrorLog("ERROR: Tiles has no properties");
                    throw std::runtime_error("Tiles has no properties");
                }
                data.tileInfo[id].isWalkable = prop["value"].get<bool>();
                int tilesetColumns = 18;
                int col = id % tilesetColumns, row = id / tilesetColumns;
                Rectangle src = {
                    (float)(col * TILESET_TILE_SIZE),
                    (float)(row * TILESET_TILE_SIZE),
                    TILESET_TILE_SIZE,
                    TILESET_TILE_SIZE
                };
                data.tileInfo[id].source = src;
            }
        }
    }

}

TilemapData LoadFromFile(const std::string& tilemap, const std::string& tileset, World& world, Resources& res) {
    TilemapData data;
    LoadTileMap(data, tilemap, world, res);
    LoadTileset(data, tileset);
    return data;
}

void Resources::Load(){
    std::string filepath = "data/system/assets.json";
    std::ifstream file(filepath);
    if(!file.is_open()){
        WriteErrorLog(std::string("ERROR: Could not verify assets: ") + filepath);
        throw std::runtime_error("Could not open asset files: " + filepath);
    }

    json dat;

    try {
        file >> dat;
    } catch (const json::exception& e) {
        WriteErrorLog(std::string("ERROR: JSON parsing error: ") + e.what());
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }

    for(const auto &txture : dat["textures"]){
        if(!txture.contains("id") || txture.empty()){
            WriteErrorLog("ERROR: textures has no texture data");
            throw std::runtime_error("textures has no texture data");
        }

        std::string asset_path, id;

        try{
            asset_path = txture["path"].get<std::string>();
            id = txture["id"].get<std::string>();
        } catch(const json::exception &e) {
            WriteErrorLog(std::string("ERROR: Texture does not have a valid id or path: ") + e.what());
            throw std::runtime_error("Texture does not have a valid id or path: " + std::string(e.what()));
        }

        Texture2D t = LoadTexture(asset_path.c_str());
        if(t.id == 0){
            WriteErrorLog(std::string("ERROR: Texture at specified path does not exist: ") + asset_path);
            throw std::runtime_error("Texture at specified path does not exist: " + asset_path);
        }

        texture.insert({id, t});
    }

    for(const auto &msc : dat["musics"]){
        if(!msc.contains("id") || msc.empty()){
            WriteErrorLog("ERROR: Musics has no music data");
            throw std::runtime_error("Musics has no music data");
        }

        std::string asset_path, id;

        try{
            asset_path = msc["path"].get<std::string>();
            id = msc["id"].get<std::string>();
        } catch(const json::exception &e) {
            WriteErrorLog(std::string("ERROR: Music does not have a valid id or path: ") + e.what());
            throw std::runtime_error("Music does not have a valid id or path: " + std::string(e.what()));
        }

        Music m = LoadMusicStream(asset_path.c_str());
        if(m.stream.buffer == nullptr){
            WriteErrorLog(std::string("ERROR: Music at specified path does not exist: ") + asset_path);
            throw std::runtime_error("Music at specified path does not exist: " + asset_path);
        }

        music.insert({id, m});
    }

    for(const auto &fx : dat["sfxs"]){
        if(!fx.contains("id") || fx.empty()){
            WriteErrorLog("ERROR: sfxs has no sfx data");
            throw std::runtime_error("sfxs has no sfx data");
        }

        std::string asset_path, id;

        try{
            asset_path = fx["path"].get<std::string>();
            id = fx["id"].get<std::string>();
        } catch(const json::exception &e) {
            WriteErrorLog(std::string("ERROR: sfx does not have a valid id or path: ") + e.what());
            throw std::runtime_error("sfx does not have a valid id or path: " + std::string(e.what()));
        }

        Sound s = LoadSound(asset_path.c_str());
        if(s.stream.buffer == nullptr){
            WriteErrorLog(std::string("ERROR: sfx at specified path does not exist: ") + asset_path);
            throw std::runtime_error("sfx at specified path does not exist: " + asset_path);
        }

        sfx.insert({id, s});
    }

    dat.clear();
    file.close();
}

Resources::~Resources(){
    for(auto &item : texture){
        UnloadTexture(item.second);
    }

    for(auto &item : music){
        UnloadMusicStream(item.second);
    }

    for(auto &item : sfx){
        UnloadSound(item.second);
    }
}

static void LoadCharacterFromFile(GameData &data, const char *filepath){
    std::ifstream file(filepath);
    if(!file.is_open()){
        WriteErrorLog(std::string("ERROR: Could not open character file: ") + filepath);
        throw std::runtime_error("Could not open character file: " + std::string(filepath));
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        WriteErrorLog(std::string("ERROR: JSON parsing error in character file: ") + e.what());
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }

    Character character;
    character.name = j.at("name").get<std::string>();
    character.portrait_id = j.at("portrait_id").get<std::string>();
    character.portrait = data.res.GetTexture(character.portrait_id);    

    const auto &stats = j.at("stats");
    character.stats.ATK = stats.at("ATK").get<int>();
    character.stats.HP = stats.at("HP").get<int>();
    character.stats.DEF = stats.at("DEF").get<float>();
    character.stats.CRIT_RATE = stats.at("CRIT_RATE").get<float>();
    character.stats.CRIT_DMG = stats.at("CRIT_DMG").get<float>();
    for(const auto &skillJson : j.at("skills")){
        Skill skill;
        skill.type = skillJson.at("type").get<std::string>();
        skill.title = skillJson.at("title").get<std::string>();
        skill.desc = skillJson.at("desc").get<std::string>();
        skill.id = skillJson.at("icon_id").get<std::string>();
        skill.icon = data.res.GetTexture(skill.id);
        character.skills.push_back(skill);
    }
    data.characters.push_back(std::move(character));
}

void LoadCharacters(GameData &data) {
    data.characters.clear();
    FilePathList files = LoadDirectoryFiles("data/characters/");
    for(int i = 0; i < files.count; i++){
        LoadCharacterFromFile(data, files.paths[i]);
    }
    UnloadDirectoryFiles(files);
}

void LoadQuests(GameData &data) {
    data.quests.clear();
    std::string filepath = "data/quests/quests.json";
    std::ifstream file(filepath);
    if(!file.is_open()){
        WriteErrorLog(std::string("ERROR: Could not open quests file: ") + filepath);
        throw std::runtime_error("Could not open quests file: " + std::string(filepath));
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        WriteErrorLog(std::string("ERROR: JSON parsing error in quests file: ") + e.what());
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }

    Quest q;

    for(auto &quest : j["quests"]){
        q.questTitle = quest["title"].get<std::string>();
        q.questDesc = quest["desc"].get<std::string>();
        q.questTask = quest["task"].get<std::string>();
        q.id = quest["id"].get<std::string>();
        for(auto &reward : quest["rewards"]){
            std::string id = reward["id"].get<std::string>();
            int amt = reward["quantity"].get<int>();
            Rewards r;
            r.item = data.inventory.GetItem(id);
            r.amount = amt;
            q.rewards.push_back(r);
        }

        for(auto &ucondition : quest["unlock_conditions"]){
            std::string type = ucondition["type"].get<std::string>();
            if(type == "talk_to_npc") q.unlockEvent.event = EventType::TALK_TO_NPC;
            if(type == "kill") q.unlockEvent.event = EventType::KILL;
            if(type == "collect_item") q.unlockEvent.event = EventType::COLLECT_ITEM;
            if(type == "complete_quest") q.unlockEvent.event = EventType::COMPLETE_QUEST;

            std::string target = ucondition["target"].get<std::string>();
            q.unlockEvent.target_id = target;
            if(ucondition.contains("amount")) q.unlockEvent.amount = ucondition["amount"].get<int>();
        }

        for(auto &endCondition : quest["end_conditions"]){
            std::string type = endCondition["type"].get<std::string>();
            if(type == "talk_to_npc") q.endEvent.event = EventType::TALK_TO_NPC;
            if(type == "kill") q.endEvent.event = EventType::KILL;
            if(type == "collect_item") q.endEvent.event = EventType::COLLECT_ITEM;
            if(type == "complete_quest") q.endEvent.event = EventType::COMPLETE_QUEST;

            std::string target = endCondition["target"].get<std::string>();
            q.endEvent.target_id = target;
            if(endCondition.contains("amount")) q.endEvent.amount = endCondition["amount"].get<int>();
        }
        q.rewardCount = q.rewards.size();
        data.quests.push_back(q);
    }
}

void LoadDailyTasks(GameData &data) {
    // data.dailyTasks.clear();
    // Quest q;
    // q.Load("KILL 100 ENEMIES", "", "", 3, std::vector<int>({1, 2, 3}));
    // data.dailyTasks.push_back(q);
    // q.Load("KILL 100 ENEMIES", "", "", 2, std::vector<int>({1, 2, 3}));
    // data.dailyTasks.push_back(q);
    // q.Load("KILL 100 ENEMIES", "", "", 3, std::vector<int>({1, 2, 3}));
    // data.dailyTasks.push_back(q);
    // q.Load("KILL 100 ENEMIES", "", "", 4, std::vector<int>({1, 2, 3}));
    // data.dailyTasks.push_back(q);
    // q.Load("KILL 100 ENEMIES", "", "", 3, std::vector<int>({1, 2, 3}));
    // data.dailyTasks.push_back(q);
}
void LoadInventory(GameData &data) {
    data.inventory.items.clear();
    data.inventory.slots.clear();

    std::string filepath = "data/item/misc.json";
    std::ifstream file(filepath);
    if(!file.is_open()){
        WriteErrorLog(std::string("ERROR: Could not open item data file: ") + filepath);
        throw std::runtime_error("Could not open item data file: " + std::string(filepath));
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e) {
        WriteErrorLog(std::string("ERROR: JSON parsing error in item data file: ") + e.what());
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }

    InventoryItem i;
    for(auto &item: j["misc"]){
        i.category = ItemCategory::MISC;
        i.description = item["desc"];
        i.icon_id = item["icon_id"];
        i.icon = data.res.GetTexture(i.icon_id);
        i.name = item["name"];
        std::string id = item["id"];
        data.inventory.items[id] = i;
    }

}
void GameData::LoadAll() {
    res.Load();
    LoadInventory(*this);
    LoadCharacters(*this);
    LoadQuests(*this);
    LoadDailyTasks(*this);
}
