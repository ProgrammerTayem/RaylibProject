#pragma once
#include <vector>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include "raylib.h"

class World;

using json = nlohmann::json;

struct TileProperties {
    Rectangle source{};
    std::string textureId;
    bool blocksPath = false;
};

struct TileLayer {
    std::string name;
    std::vector<int> data;

    int GetTile(int x, int y, int width) const {
        if (x < 0 || y < 0 || data.empty()) return 0;
        if (x >= width || y >= static_cast<int>(data.size() / width)) return 0;
        return data[y * width + x];
    }
};

struct CollisionRect {
    std::string name;
    Rectangle bounds{};
};

struct TriggerRect {
    std::string name;
    Rectangle bounds{};
    int value = 0;
};

struct TilemapData {
    int width = 0;
    int height = 0;
    int tileWidth = 32;
    int tileHeight = 32;
    std::vector<TileLayer> layers;
    std::vector<std::string> renderLayerNames;
    std::map<std::string, size_t> layerIndex;
    std::map<int, TileProperties> gidProperties;
    std::vector<CollisionRect> collisionRects;
    std::vector<TriggerRect> triggerRects;

    const TileLayer* GetLayer(const std::string& name) const {
        auto it = layerIndex.find(name);
        if (it == layerIndex.end()) return nullptr;
        return &layers[it->second];
    }

    int GetGid(const std::string& layerName, int x, int y) const {
        const TileLayer* layer = GetLayer(layerName);
        if (!layer) return 0;
        return layer->GetTile(x, y, width);
    }

    const TileProperties& GetTileProps(int gid) const {
        static const TileProperties empty{};
        if (gid <= 0) return empty;
        auto it = gidProperties.find(gid);
        if (it == gidProperties.end()) return empty;
        return it->second;
    }
};

void LoadTileMap(TilemapData& data, const std::string& tilemap, World& world, struct Resources& res);
void LoadTilesets(TilemapData& data, const std::string& tilemap, struct Resources& res);
TilemapData LoadFromFile(const std::string& tilemap, World& world, struct Resources& res);
std::string ResolveDataPath(const std::string& relativePath);
