#pragma once
#include "scene.h"
#include "Loader.h"
#include "animations.h"
#include "gameData.h"
#include <raylib.h>
#include <vector>
#include <string>
#include <optional>

#define TILE_SIZE 32
#define TILESET_TILE_SIZE 16

class World;

class Player{

    public:
        explicit Player(Resources &res);
        ~Player() = default;
        void Update(float dt, World& world);
        void Draw() const;
        Vector2 GetPosition() const { return position; }
        int GetElevation() const { return elevation; }
        void SetPosition(Vector2 pos) { position = pos; }

    private:
        enum AnimList {RUN_LEFT, RUN_UP, RUN_RIGHT, RUN_DOWN, IDLE_LEFT, IDLE_UP, IDLE_RIGHT, IDLE_DOWN};
        const int ANIMS = 8, SPRITES = 8;
        std::vector<Animation> animations;
        std::vector<Texture2D*> sprites;
        int curAnimation, spriteFrame;
        int elevation = 1;
        float speed;
        Vector2 position;
};

class Entity {

    public:
        virtual ~Entity() = default;
        virtual void Update(float dt, Player& player) = 0;
        virtual void Draw() const = 0;

    protected:
        Vector2 position;
};

class InteractiveEntity : public Entity {

    public:
        virtual ~InteractiveEntity() = default;
        int HandleInteraction(Vector2 playerPos, float dt);

    protected:

        std::string name, id;
        std::vector<std::string> dialogueLines;
        bool canInteract, dialogueActive;
        int playerRelPosition, dialogueIndex;
};

class NPC : public InteractiveEntity {

    public:
        NPC();
        NPC MakeNPC(std::string& name, Vector2 position, std::vector<std::string>& DialogueLines);
        void Update(float dt, Player& player) override;
        void Draw() const override;
};

class Enemy : public InteractiveEntity {

    public:
        explicit Enemy(Resources &res);
        Enemy(Resources &res, const Vector2 position, const std::string &name,
              const std::vector<std::string>& dialogueLines = {}, std::array<Character*, 3> inBattleCharacters = {}, const std::string textureId = "enemy");
        ~Enemy() = default;
        void Update(float dt, Player& player) override;
        void Draw() const override;

    private:
        enum AnimList {IDLE};
        const int ANIMS = 1, SPRITES = 1;
        std::vector<Animation> animations;
        std::vector<Texture2D*> sprites;
        int curAnimation, spriteFrame;
        std::array<Character*, 3> inBattleCharacters;

};


class World{

    public:
        explicit World(Resources &res);
        ~World();
        void LoadFromTilemap(const std::string& filepath, World& world);
        void Update(float dt, Player &player);
        void Draw(const Player &player);
        void DrawCollisionDebug(Vector2 playerPos) const;
        bool TryMove(Vector2& position, Vector2 nextPosition, int& elevation) const;
        Vector2 FindSpawnPosition() const;
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        std::vector<std::unique_ptr<Entity>>entities;

    private:
        bool IsWalkableAtElevation(int x, int y, int elevation) const;
        bool IsStairTile(int x, int y) const;
        bool IsGroundPathTile(int x, int y) const;
        bool IsBlockedByCollision(Rectangle playerBounds, int elevation) const;
        bool TileBlocksPath(int gid) const;

        Resources &res;
        int width = 30, height = 30;
        TilemapData tilemapData;
};

class Typewriter{

    public:
        void TypeWriterUpdate(const std::string& text, float dt);
        void ResetTypewriter() { typewriterFinished = false; currentText.clear(); renderingPart.clear(); charIndex = 0; timer = 0.0f; }
        void CompleteTypewriter(const std::string& text) { charIndex = text.size(); renderingPart = text; typewriterFinished = true; }
        bool IsTypewriterComplete(const std::string& text) const { return renderingPart == text; }
        void Draw() const;
        bool isActive = false;

    private:
        bool typewriterFinished = false;
        int charIndex = 0;
        float timer = 0.0f;
        std::string renderingPart, currentText;
};

extern Typewriter typeWriter;

class GUI{

    public:
        GUI(bool setBlocker, GameData &data, bool drawWorld = false) : blockProgress(setBlocker), drawWorldUnderneath(drawWorld), gameData(data) {}
        virtual ~GUI() = default;
        virtual GUI* Update(float dt) = 0;
        virtual void Draw() = 0;
        bool blockProgress;
        bool drawWorldUnderneath;

    protected:
        GameData &gameData;

};

class OverlayUI : public GUI{

    public:
        explicit OverlayUI(GameData &data);
        ~OverlayUI() = default;
        GUI* Update(float dt) override;
        void Draw() override;
        void TransitionBreatheInOut(Rectangle dest, Color peak);

    private:
        Texture2D *charactersIcon = nullptr, *dailyTasksIcon = nullptr, *questsIcon = nullptr, *inventoryIcon = nullptr, *newQuest = nullptr;
        float time = 0, transitionLife = 1.5f;
        char shortCutKey[4] = {'C', 'T', 'Q', 'I'};

};

class InventoryUI : public GUI{

    public:
        explicit InventoryUI(GameData &data);
        ~InventoryUI() = default;
        GUI* Update(float dt) override;
        void Draw() override;

    private:
        typedef ItemCategory Tabs;
        bool crossClicked;
        int clickedTab, showDesc;
        Tabs selectedTab;
        std::vector<std::string> tabText;
        Texture2D *panels = nullptr;
        const int rows = 10, cols = 20;
        int items[10][20];

};

class QuestsUI : public GUI{

     public:
        explicit QuestsUI(GameData &data);
        ~QuestsUI() = default;
        GUI* Update(float dt) override;
        void Draw() override;

    private:
        bool crossClicked, trackClicked, collectClicked;
        int selectedQuest, track;
        Texture2D *panels = nullptr;
};

class DailyTasksUI : public GUI{

    public:
        explicit DailyTasksUI(GameData &data);
        ~DailyTasksUI() = default;
        GUI* Update(float dt) override;
        void Draw() override;

    private:
        bool crossClicked;
        Texture2D *panels = nullptr;
};


class CharacterUI : public GUI{

    public:
        explicit CharacterUI(GameData &data);
        ~CharacterUI() = default;
        GUI* Update(float dt) override;
        void Draw() override;

    private:
        bool crossClicked;
        int selectedCharacter, selectedSkill;
        Texture2D *atk = nullptr;
        Texture2D *def = nullptr;
        Texture2D *hp = nullptr;
        Texture2D *crit_rate = nullptr;
        Texture2D *crit_dmg = nullptr;
};

class PauseUI : public GUI{

    public:
        explicit PauseUI(GameData &data);
        ~PauseUI() = default;
        GUI* Update(float dt) override;
        void Draw() override;

    private:
        bool resumeClicked;
        bool exitClicked;
};


struct Cam{

    Camera2D cam;
    float smoothSpeed = 5.0f;
    int mapWidthPixels = 30 * TILE_SIZE;
    int mapHeightPixels = 30 * TILE_SIZE;

    Cam() {
        cam.target = { 0.0f, 0.0f };
        cam.offset = { 0.0f, 0.0f };
        cam.rotation = 0.0f;
        cam.zoom = 2.0f;
    }

    void SetMapSize(int width, int height);
    void SetTarget(Vector2 target) { cam.target = target; }
    void Update(float dt, const Vector2& playerPos);
};



class GameScene : public Scene{

    public:
        GameScene(Vector2 spawnPos = {-1, -1});
        ~GameScene();
        Scene* Update(float dt) override;
        void Draw() override;

    private:
        Cam camera;
        World world;
        Player player;
        GUI *gui;
        Music* background;
        bool showCollisionDebug = false;
};
