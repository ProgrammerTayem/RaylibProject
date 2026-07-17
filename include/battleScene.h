#pragma once
#include "scene.h"
#include "animations.h"
#include "gameData.h"
#include <raylib.h>
#include <vector>
#include <string>
#include <optional>

struct SkillVM;

struct C{
    Stats stats;
    std::vector<Skill> skills;
    std::string name;
    Texture2D *portrait = nullptr;
};

struct BattleTeam{
    std::array<C, 3> members;
    int activeIndex = 0, switchCooldown = 0;
    bool forceSwitch = false;
    C& active(){ return members[activeIndex]; }
    const C& active() const{ return members[activeIndex]; }
};

enum class BattleState{
    Entering,
    BeginTurn,
    WaitingForInput,
    SelectingSkill,
    SwitchingCharacter,
    ExecutingSkill,
    ExecutingAnimation,
    ProcessingSkillEffect,
    EndTurn,
    Victory,
    Defeat,
    Leaving
};

enum class TargetType{
    Self,
    Ally,
    Enemy,
    AllAllies,
    AllEnemies,
    Everyone
};

enum class BattleActionType
{
    Switch,
    Skill,
    Item,
    Escape
};

enum class BattleSide{
    Player,
    Enemy
};

struct BattleAction{
    BattleActionType bActType;
    C *actor, *target;
    Skill *skill;
    InventoryItem* item;
};

struct BattleContext{
    BattleTeam player, enemy;
    BattleSide currentTurn;
    BattleState state = BattleState::Entering;
    BattleAction pendingAction;
};

struct UILayout{
    Rectangle skillButton, characterSelector, switchCharacter, skipTurn;
    bool skillButtonPressed, characterSelectorPressed, switchCharacterPressed, skipTurnPressed;
};

class BattleUI{
    public:
        BattleUI();
        void Update(BattleContext& ctx, float dt);
        void Draw(const BattleContext& ctx) const;

    private:
        UILayout layout;
        float timer = 0.0f, IntroOut = 2.6f;
        int selectedSkill = 0, selectedTarget = 0;
        bool showPlayerDependantUi = true;
        float skillButtonOffset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        void UpdateSkillButtonOffsets(const BattleContext& ctx, float dt);
};

class BattleRenderer{

    public:
        void Draw(const BattleContext& ctx) const;
};


class BattleScene : public Scene{

    public:
        BattleScene();
        ~BattleScene();
        Scene* Update(float dt) override;
        void Draw() override;
        void BeginTurn();
        void HandleInput(float dt);
        void ExecuteAction();
        void ProcessEffects();
        void EndTurn();

    private:
        BattleContext battle;
        BattleUI ui;
        BattleRenderer renderer;
};

bool IntroAnimation(float dt);