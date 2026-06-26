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

    private:
        enum AnimList {RUN_LEFT, RUN_UP, RUN_RIGHT, RUN_DOWN, IDLE_LEFT, IDLE_UP, IDLE_RIGHT, IDLE_DOWN};
        const int ANIMS = 8, SPRITES = 8;
        std::vector<Animation> animations;
        std::vector<Texture2D*> sprites;
        int curAnimation, spriteFrame;
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
        bool DialogueActive() const { return dialogueActive; }
        int DialogueIndex() const { return dialogueIndex; }

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
        Enemy(Resources &res, const Vector2 position, const std::string &name);
        ~Enemy() = default;
        void Update(float dt, Player& player) override;
        void Draw() const override;

    private:
        enum AnimList {IDLE};
        const int ANIMS = 1, SPRITES = 1;
        std::vector<Animation> animations;
        std::vector<Texture2D*> sprites;
        int curAnimation, spriteFrame;

};


class World{

    public:
        explicit World(Resources &res);
        ~World();
        void LoadFromTilemap(const std::string& filepath, World& world);
        void Update(float dt, Player &player);
        void Draw();
        bool IsWalkable(int x, int y);
        int GetWidth() const { return width; }
        int GetHeight() const { return height; }
        std::vector<std::unique_ptr<Entity>>entities;

    private:
        Resources &res;
        int width = 150, height = 150;
        TilemapData tilemapData;
        Texture2D *tilesetTexture = nullptr;
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
        GUI(bool setBlocker, GameData &data) : blockProgress(setBlocker), gameData(data) {}
        virtual ~GUI() = default;
        virtual GUI* Update(float dt) = 0;
        virtual void Draw() = 0;
        bool blockProgress;

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


struct Cam{

    Camera2D cam;
    float smoothSpeed = 5.0f;
    int mapWidthPixels = 150 * TILE_SIZE;
    int mapHeightPixels = 150 * TILE_SIZE;
    const float viewWidth = 800.0f / cam.zoom, viewHeight = 450.0f / cam.zoom;

    Cam() {
        cam.target = { 0.0f, 0.0f };
        cam.offset = { 400.0f, 225.0f };
        cam.rotation = 0.0f;
        cam.zoom = 1.5f;
    }

    void SetMapSize(int width, int height) {
        mapWidthPixels = width * TILE_SIZE;
        mapHeightPixels = height * TILE_SIZE;
    }

    void Update(float dt, const Vector2& playerPos);
};



class GameScene : public Scene{

    public:
        GameScene();
        ~GameScene();
        Scene* Update(float dt) override;
        void Draw() override;

    private:
        Cam camera;
        World world;
        Player player;
        GUI *gui;
        Music* background;
};

