#include "Loader.h"
#include "gameScene.h"
#include "gameData.h"
#include <fstream>
#include <stdexcept>
#include <ctime>
#include <filesystem>
#include <algorithm>
#include <climits>
#include <cctype>

GameData gameData;
static void WriteErrorLog(const std::string& message);

/*
Return texture specified by const std::string id, or write to the error log and return nullptr if not found
Parameters:
    id: const std::string - the id of the texture to retrieve
*/
Texture2D *Resources::GetTexture(const std::string &id){
    auto it = texture.find(id);
    if(it == texture.end()){
        WriteErrorLog("ERROR: Texture id not found: " + id);
        return nullptr;
    }
    return &it->second;
}

/*
Return sound effect specified by const std::string id, or write to the error log and return nullptr if not found
Parameters:
    id: const std::string - the id of the sound effect to retrieve
*/
Sound *Resources::GetSfx(const std::string &id){
    auto it = sfx.find(id);
    if(it == sfx.end()){
        WriteErrorLog("ERROR: Sfx id not found: " + id);
        return nullptr;
    }
    return &it->second;
}
/*
Return music specified by const std::string id, or write to the error log and return nullptr if not found
Parameters:
    id: const std::string - the id of the music to retrieve
*/
Music *Resources::GetMusic(const std::string &id){
    auto it = music.find(id);
    if(it == music.end()){
        WriteErrorLog("ERROR: Music id not found: " + id);
        return nullptr;
    }
    return &it->second;
}

/*
Return a pointer to the Character with the specified id, or write to the error log and return nullptr if not found
Parameters:
    id: const std::string - the id of the character to retrieve
*/
Character* GameData::GetCharacter(const std::string &id){
    auto it = std::find_if(characters.begin(), characters.end(), [&id](const Character& character) {
        return character.name == id;
    });
    if(it == characters.end()){
        WriteErrorLog("ERROR: Character id not found: " + id);
        return nullptr;
    }
    return &*it;
}

/*
Write a message to the error log
Parameters:
    message: const std::string - the message to write to the error log
*/
static void WriteErrorLog(const std::string& message) {
    std::ofstream logFile("ErrorLog", std::ios::app);
    if(logFile.is_open()){
        std::time_t now = std::time(nullptr);
        char timestamp[64];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        logFile << "[" << timestamp << "] " << message << '\n';
    }
}

/*
Normalize a data path by ensuring it starts with "data/"
Parameters:
    path: const std::string - the path to normalize
Returns:
    std::string - the normalized path
*/
static std::string NormalizeDataPath(const std::string& path) {
    const auto pos = path.find("data/");
    if(pos != std::string::npos) return path.substr(pos);
    const auto posBack = path.find("data\\");
    if(posBack != std::string::npos) {
        std::string normalized = path.substr(posBack);
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        return normalized;
    }
    return path;
}

/*
Get a property value from a JSON object
Parameters:
    obj: const json& - the JSON object to search
    propName: const std::string& - the name of the property to retrieve
Returns:
    T - the value of the property, or a default-constructed T if not found
*/
template<typename T>
static T GetObjectProperty(const json& obj, const std::string& propName, const T& defaultValue = T()) {
    if(!obj.contains("properties")) return defaultValue;
    for(const auto& prop : obj["properties"]){
        if(prop.value("name", "") == propName) {
            if(prop["value"].is_string()) return prop["value"].get<T>();
            return prop["value"].get<T>();
        }
    }
    return defaultValue;
}

/*
Load dialogue lines from a JSON file
Parameters:
    dialoguePath: const std::string& - the path to the dialogue file
Returns:
    std::vector<std::string> - the loaded dialogue lines
*/
static std::vector<std::string> LoadDialogueLines(const std::string& dialoguePath) {
    const std::string normalizedPath = NormalizeDataPath(dialoguePath);
    std::ifstream dialogue(normalizedPath);
    if(!dialogue.is_open()){
        WriteErrorLog("ERROR: Could not open dialogue file: " + normalizedPath);
        throw std::runtime_error("Could not open dialogue file: " + normalizedPath);
    }

    json dialogueJson;
    try {
        dialogue >> dialogueJson;
    } catch (const json::exception& e){
        WriteErrorLog(std::string("ERROR: JSON parsing error: ") + e.what());
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }

    return dialogueJson["lines"].get<std::vector<std::string>>();
}

/*
Strip flags from a GID
Parameters:
    gid: int - the GID to strip flags from
Returns:
    int - the GID with flags stripped
*/
static int StripGidFlags(int gid) {
    return gid & 0x1FFFFFFF;
}

/*
Resolve a data path by searching for it in common locations
Parameters:
    relativePath: const std::string& - the relative path to resolve
Returns:
    std::string - the resolved path
*/
static std::string ResolveDataPath(const std::string& relativePath) {
    if(std::filesystem::exists(relativePath)) return relativePath;

    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::path("..") / relativePath,
        std::filesystem::path("..") / ".." / relativePath,
    };

    for(const auto& candidate : candidates){
        if(std::filesystem::exists(candidate)) return candidate.string();
    }

    return relativePath;
}

/*
Find a JSON layer by name
Parameters:
    layers: const json& - the JSON object containing layers
    name: const std::string& - the name of the layer to find
Returns:
    const json* - a pointer to the found layer, or nullptr if not found
*/
static const json* FindJsonLayerByName(const json& layers, const std::string& name) {
    for(const auto& layer : layers){
        if(layer.value("name", "") == name) return &layer;
    }
    return nullptr;
}

/*
Get the texture ID for a given image name
Parameters:
    imageName: const std::string& - the name of the image
Returns:
    std::string - the corresponding texture ID
*/
static std::string TextureIdForImage(const std::string& imageName) {

    static const std::map<std::string, std::string> imageToTexture = {
        {"Topdown RPG 32x32 - Ruins.PNG", "ruins_tileset"},
        {"Topdown RPG 32x32 - WaterTileset.PNG", "water_tileset"},
        {"Topdown RPG 32x32 - Ground Tileset 1.2.PNG", "ground_tileset"},
        {"Topdown RPG 32x32 - Tree Stumps and Logs 1.2.PNG", "tree_stumps_logs_tileset"},
        {"Topdown RPG 32x32 - Trees 1.2.PNG", "trees_tileset"},
        {"Topdown RPG 32x32 - Bushes 1.1.PNG", "bushes_tileset"},
    };

    const std::filesystem::path imagePath(imageName);
    const std::string fileName = imagePath.filename().string();

    auto normalize = [](std::string value){
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
        return value;
    };

    const std::string lowerImageName = normalize(imageName);
    const std::string lowerFileName = normalize(fileName);

    for(const auto& [image, textureId] : imageToTexture) {
        const std::string lowerKnown = normalize(image);
        if(lowerKnown == lowerImageName || lowerKnown == lowerFileName) {
            return textureId;
        }
    }

    WriteErrorLog("ERROR: No texture id mapping for tileset image: " + imageName);
    throw std::runtime_error("No texture id mapping for tileset image: " + imageName);
}

void LoadTileMap(TilemapData& data, const std::string& tilemap, World& world, Resources& res){
    const std::string resolvedTilemap = ResolveDataPath(tilemap);
    std::ifstream file(resolvedTilemap);
    if(!file.is_open()){
        WriteErrorLog("ERROR: Could not open tilemap file: " + resolvedTilemap);
        throw std::runtime_error("Could not open tilemap file: " + resolvedTilemap);
    }

    json j;
    try {
        file >> j;
    } catch (const json::exception& e){
        WriteErrorLog(std::string("ERROR: JSON parsing error: ") + e.what());
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
    }

    // ------------
    // Main loading begins here
    // ------------


    // -----
    // Metadata
    // -----

    data.width = j.value("width", 0);
    data.height = j.value("height", 0);
    data.tileWidth = j.value("tilewidth", TILE_SIZE);
    data.tileHeight = j.value("tileheight", TILE_SIZE);

    if(data.width <= 0 || data.height <= 0){
        const std::string message = "ERROR: Invalid tilemap dimensions: " +
                                    std::to_string(data.width) + "x" +
                                    std::to_string(data.height);
        WriteErrorLog(message);
        throw std::runtime_error(message);
    }

    if(!j.contains("layers") || j["layers"].empty()){
        WriteErrorLog("ERROR: Tilemap has no layers");
        throw std::runtime_error("Tilemap has no layers");
    }

    static const std::vector<std::string> kRenderLayerOrder = {
        "Path", "Shadows", "Platform", "Obstacles", "Treetops", "Decors", "Upper Obstacle"
    };

    data.layers.clear();
    data.layerIndex.clear();
    data.renderLayerNames.clear();
    data.collisionRects.clear();

    // ------------------
    // Visible tile layers
    // ------------------

    for(const auto& layerName : kRenderLayerOrder){
        const json* layerJson = FindJsonLayerByName(j["layers"], layerName);
        if(!layerJson || layerJson->value("type", "") != "tilelayer") continue;

        TileLayer layer;
        layer.name = layerName;
        layer.data = layerJson->at("data").get<std::vector<int>>();
        for (int& gid : layer.data) gid = StripGidFlags(gid);

        if(layer.data.size() != static_cast<size_t>(data.width * data.height)){
            const std::string message = "ERROR: Layer '" + layerName + "' size mismatch";
            WriteErrorLog(message);
            throw std::runtime_error(message);
        }

        data.layerIndex[layerName] = data.layers.size();
        data.layers.push_back(std::move(layer));
        data.renderLayerNames.push_back(layerName);
    }

    // ------------------
    // Collision layers
    // ------------------

    const json* collisionLayer = FindJsonLayerByName(j["layers"], "Collision");
    if(collisionLayer && collisionLayer->contains("objects")) {
        for (const auto& obj : collisionLayer->at("objects")) {
            CollisionRect rect;
            rect.name = obj.value("name", "");
            rect.bounds = {
                obj.value("x", 0.0f),
                obj.value("y", 0.0f),
                obj.value("width", 0.0f),
                obj.value("height", 0.0f)
            };
            data.collisionRects.push_back(rect);
        }
    }

    // ------------------
    // Trigger layer
    // ------------------

    const json* triggerLayer = FindJsonLayerByName(j["layers"], "Triggers");
    if(triggerLayer && triggerLayer->contains("objects")) {
        for (const auto& obj : triggerLayer->at("objects")) {
            TriggerRect rect;
            rect.name = obj.value("name", "");
            rect.bounds = {
                obj.value("x", 0.0f),
                obj.value("y", 0.0f),
                obj.value("width", 0.0f),
                obj.value("height", 0.0f)
            };
            for(const auto& prop : obj.value("properties", json::array())) {
                if(prop.contains("name") && prop["name"] == "value" && prop.contains("value")) {
                    rect.value = prop["value"].get<int>();
                    break;
                }
            }
            data.triggerRects.push_back(rect);
        }
    }

    // ------------------
    // Entities layer
    // ------------------

    const json* entitiesLayer = FindJsonLayerByName(j["layers"], "Entities");
    if(!entitiesLayer || !entitiesLayer->contains("objects")){
        WriteErrorLog("WARNING: Entities layer has no objects");
        throw std::runtime_error("Entities layer has no objects");
    }

    for (const auto& obj : entitiesLayer->at("objects")) {
        if (!obj.contains("type")) {
            WriteErrorLog("WARNING: Entity object has no type");
            throw std::runtime_error("Entity object has no type");
        }

        const std::string type = obj["type"].get<std::string>();
        const std::string name = obj["name"].get<std::string>();
        const int x = obj["x"].get<int>();
        const int y = obj["y"].get<int>();
        const std::string dialoguePath = GetObjectProperty<std::string>(obj, "dialogueLines");

        // Create entities based on their type
        if (type == "NPC") {
            std::vector<std::string> dialogueLines = LoadDialogueLines(dialoguePath);
            std::string npcName = name;
            NPC npc = NPC().MakeNPC(npcName, Vector2{(float)x, (float)y}, dialogueLines);
            world.entities.push_back(std::make_unique<NPC>(std::move(npc)));


        } else if (type == "Enemy") {
            std::vector<std::string> dialogueLines;
            const std::vector<std::string> accompanyingCharacters = {
                GetObjectProperty<std::string>(obj, "characterOnMap"),
                GetObjectProperty<std::string>(obj, "character2"),
                GetObjectProperty<std::string>(obj, "character3")
            };
            std::array<Character*, 3> inBattleCharacters = {nullptr, nullptr, nullptr};
            for (size_t i = 0; i < accompanyingCharacters.size(); ++i){
                if(!accompanyingCharacters[i].empty()){
                    inBattleCharacters[i] = gameData.GetCharacter(accompanyingCharacters[i]);
                }
            }
            std::string textureID = inBattleCharacters[0]->portrait_id;
            if (!dialoguePath.empty()) dialogueLines = LoadDialogueLines(dialoguePath);
            world.entities.push_back(std::make_unique<Enemy>(res, Vector2{(float)x, (float)y}, name, dialogueLines, inBattleCharacters, textureID));
        }
    }
}

void LoadTilesets(TilemapData& data, const std::string& tilemap, Resources& res) {
    const std::string resolvedTilemap = ResolveDataPath(tilemap);
    std::ifstream file(resolvedTilemap);
    if (!file.is_open()) {
        WriteErrorLog("ERROR: Could not open tilemap file: " + resolvedTilemap);
        throw std::runtime_error("Could not open tilemap file: " + resolvedTilemap);
    }

    json j;
    file >> j;

    if (!j.contains("tilesets")) {
        WriteErrorLog("ERROR: Tilemap has no tilesets");
        throw std::runtime_error("Tilemap has no tilesets");
    }

    const std::filesystem::path tilemapPath(resolvedTilemap);
    const std::filesystem::path tilesetDir = tilemapPath.parent_path().parent_path() / "tilesets";

    struct TilesetRef {
        int firstGid;
        std::string source;
    };
    std::vector<TilesetRef> tilesetRefs;

    for (const auto& tilesetEntry : j["tilesets"]) {
        TilesetRef ref;
        ref.firstGid = tilesetEntry.value("firstgid", 0);
        ref.source = tilesetEntry.value("source", "");
        if (ref.source.empty() || ref.firstGid <= 0) continue;
        tilesetRefs.push_back(ref);
    }

    std::sort(tilesetRefs.begin(), tilesetRefs.end(),
              [](const TilesetRef& a, const TilesetRef& b) {
                  if (a.firstGid != b.firstGid) return a.firstGid < b.firstGid;
                  return a.source < b.source;
              });

    struct LoadedTileset {
        int firstGid;
        int nextFirstGid;
        std::filesystem::path path;
        std::string source;
    };
    std::vector<LoadedTileset> loadedTilesets;

    for (size_t i = 0; i < tilesetRefs.size(); ++i) {
        const int firstGid = tilesetRefs[i].firstGid;

        bool gidAlreadyLoaded = false;
        for (const auto& loaded : loadedTilesets) {
            if (loaded.firstGid == firstGid) {
                gidAlreadyLoaded = true;
                break;
            }
        }
        if (gidAlreadyLoaded) continue;

        const std::filesystem::path tilesetPath = tilesetDir / tilesetRefs[i].source;
        if (!std::filesystem::exists(tilesetPath)) {
            WriteErrorLog("WARNING: Skipping missing tileset: " + tilesetPath.string());
            continue;
        }

        int nextFirstGid = INT_MAX;
        for (size_t j = 0; j < tilesetRefs.size(); ++j) {
            if (tilesetRefs[j].firstGid > firstGid) {
                nextFirstGid = tilesetRefs[j].firstGid;
                break;
            }
        }

        loadedTilesets.push_back({
            firstGid,
            nextFirstGid,
            tilesetPath,
            tilesetRefs[i].source
        });
    }

    data.gidProperties.clear();

    for (const auto& tilesetRef : loadedTilesets) {
        std::ifstream tilesetFile(tilesetRef.path);
        if (!tilesetFile.is_open()) {
            WriteErrorLog("ERROR: Could not open tileset file: " + tilesetRef.path.string());
            throw std::runtime_error("Could not open tileset file: " + tilesetRef.path.string());
        }

        json tilesetJson;
        tilesetFile >> tilesetJson;

        const int firstGid = tilesetRef.firstGid;
        const int nextFirstGid = tilesetRef.nextFirstGid;
        const int columns = tilesetJson.value("columns", 1);
        const int tileWidth = tilesetJson.value("tilewidth", data.tileWidth);
        const int tileHeight = tilesetJson.value("tileheight", data.tileHeight);
        const int tileCount = tilesetJson.value("tilecount", 0);
        const std::string imageName = tilesetJson.value("image", "");
        const std::string textureId = TextureIdForImage(imageName);

        if (!res.GetTexture(textureId)) {
            WriteErrorLog("ERROR: Texture not loaded for tileset: " + textureId);
            throw std::runtime_error("Texture not loaded for tileset: " + textureId);
        }

        std::map<int, TileProperties> localOverrides;
        if (tilesetJson.contains("tiles")) {
            for (const auto& tile : tilesetJson["tiles"]) {
                const int localId = tile.value("id", -1);
                if (localId < 0) continue;

                TileProperties props;
                props.textureId = textureId;
                props.blocksPath = !GetObjectProperty<bool>(tile, "isWalkable", false);

                localOverrides[localId] = props;
            }
        }

        for (int localId = 0; localId < tileCount; ++localId) {
            const int gid = firstGid + localId;
            if (gid >= nextFirstGid) break;

            TileProperties props;
            const auto overrideIt = localOverrides.find(localId);
            if (overrideIt != localOverrides.end()) props = overrideIt->second;

            props.textureId = textureId;
            // if (isWaterTileset) props.isWater = true;

            const int col = localId % columns;
            const int row = localId / columns;
            props.source = {
                (float)(col * tileWidth),
                (float)(row * tileHeight),
                (float)tileWidth,
                (float)tileHeight
            };

            data.gidProperties[gid] = props;
        }
    }
}

TilemapData LoadFromFile(const std::string& tilemap, World& world, Resources& res) {
    TilemapData data;
    LoadTileMap(data, tilemap, world, res);
    LoadTilesets(data, tilemap, res);
    return data;
}

void Resources::Load(){
    std::string filepath = ResolveDataPath("data/system/assets.json");
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
    character.isEnemy = j.at("isEnemy").get<bool>();
    character.portrait = data.res.GetTexture(character.portrait_id);    

    const auto &stats = j.at("stats");
    character.stats.ATK = stats.at("ATK").get<int>();
    character.stats.HP = stats.at("HP").get<int>();
    character.stats.maxHP = stats.value("maxHP", character.stats.HP);
    character.stats.DEF = stats.at("DEF").get<float>();
    character.stats.CRIT_RATE = stats.at("CRIT_RATE").get<float>();
    character.stats.CRIT_DMG = stats.at("CRIT_DMG").get<float>();
    for(const auto &skillJson : j.at("skills")){
        Skill skill;
        skill.type = skillJson.at("type").get<std::string>();
        skill.title = skillJson.at("title").get<std::string>();
        skill.desc = skillJson.at("desc").get<std::string>();
        skill.id = skillJson.at("icon_id").get<std::string>();
        skill.target = skillJson.at("target").get<std::string>();
        skill.baseCooldown = skillJson.at("cooldown").get<int>();
        skill.cooldown = 0;
        skill.icon = data.res.GetTexture(skill.id);
        for(const auto &effect : skillJson["effects"]){
            SkillModifiers modifier;
            modifier.type = effect.at("type").get<std::string>();
            for(auto & [key, value] : effect.items()){
                try{
                    modifier.additionalParams[key] = value;
                } catch(const json::exception &e){
                    WriteErrorLog(std::string("ERROR: JSON parsing error for effects: ") + e.what());
                    throw std::runtime_error("JSON parsing error: " + std::string(e.what()));
                }
            }
            skill.modifiers.push_back(std::move(modifier));
        }
        character.skills.push_back(std::move(skill));
    }
    data.characters.push_back(std::move(character));
    if(!character.isEnemy) data.playableCharacterCount++;
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
    playableCharacterCount = 0;
    LoadInventory(*this);
    LoadCharacters(*this);
    LoadQuests(*this);
    LoadDailyTasks(*this);
}
