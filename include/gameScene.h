#pragma once
#include "scene.h"
#include <raylib.h>
#include <vector>
#include <string>

class World; 

enum TileType { GRASS, WATER, SAND };
struct Tile {
    TileType ttype;
    bool isWalkable;
};

class Entity {
    public:
        virtual ~Entity() = default;
        virtual void Update(float dt, World& world) = 0;
        virtual void Draw() const = 0;
    protected:
        Vector2 position;
};

class Player : public Entity {
    public:
        Player();
        void Update(float dt, World& world) override;
        void Draw() const override;
        Vector2 GetPosition() const { return position; }
    private:
        float speed;
};

class NPC : public Entity {
    public:
        NPC();
        void Update(float dt, World& world) override;
        void Draw() const override;
        Vector2 GetPosition() const { return position; }
        void SetPosition(const Vector2& pos) { position = pos; }
        void HandleInteraction(Player& player);
        bool IsDialogueActive() const { return dialogueActive; }
        const std::string& GetDialogue() const { return dialogue; }
        std::string GetName() const { return name; }
    private:
        std::string name, dialogue;
        bool dialogueActive, canInteract;
        int playerRelPosition;
};

class World{
    public:
        void GenerateTestMap();
        void Update(float dt);
        void Draw();
        bool IsWalkable(int x, int y);
    private:
        static const int width = 200, height = 200;
        static const int entityCount = 10;
        Tile map[width][height];
};

class UI{
    public:
        void TypewriterEffect(const std::string& text, int x, int y, int fontSize, Color color, float dt);
        void DeactivateTypewriter() { typewriterFinished = true; }
        void ResetTypewriter() { typewriterFinished = false; charIndex = 0; timer = 0.05f; }
    protected:
        bool typewriterFinished = false;
        int charIndex = 0;
        float timer = 0.05f;

};

class GameScene : public Scene{
    public:
        GameScene();
        Scene* Update(float dt) override;
        void Draw() override;
    private:
        float smoothSpeed = 5.0f;
        bool typewriterActive = false;
        Camera2D camera;
        UI ui;
        World world;
        Player player;
        std::vector<NPC> npcs;
};