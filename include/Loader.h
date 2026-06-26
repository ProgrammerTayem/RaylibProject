#pragma once
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <map>
#include "raylib.h"

class World;

using json = nlohmann::json;

struct tileInfo{
    bool isWalkable, isDestructible;
    Rectangle source;
};

struct TilemapData {
    int width;
    int height;
    std::vector<int> tileData;
    std::map<int, tileInfo> tileInfo;
    
    int GetTile(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) return 0;
        if (tileData.empty()) return 0;
        return tileData[y * width + x];
    }
    
    void SetTile(int x, int y, int tileId) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        tileData[y * width + x] = tileId;
    }
};

void LoadTileMap(TilemapData &data, const std::string& tilemap, World& world, struct Resources& res);
void LoadTileset(TilemapData &data, const std::string& tileset);
TilemapData LoadFromFile(const std::string& tilemap, const std::string& tileset, World& world, struct Resources& res);
