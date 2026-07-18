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
    std::array<bool, 3> defeated = {};
    int activeIndex = 0, switchCooldown = 0;
    bool forceSwitch = false;
    C& active(){ return members[activeIndex]; }
    const C& active() const{ return members[activeIndex]; }
    bool IsAvailable(int i) const {
        return i >= 0 && i < 3 && i != activeIndex && !defeated[i] && !members[i].name.empty();
    }
    int AvailableCount() const {
        int n = 0;
        for(int i = 0; i < 3; i++) if(IsAvailable(i)) n++;
        return n;
    }
    int FirstAvailable() const {
        for(int i = 0; i < 3; i++) if(IsAvailable(i)) return i;
        return -1;
    }
    int RandomAvailable() const {
        int count = AvailableCount();
        if(count <= 0) return -1;
        int pick = GetRandomValue(0, count - 1);
        int seen = 0;
        for(int i = 0; i < 3; i++){
            if(IsAvailable(i) && seen++ == pick) return i;
        }
        return -1;
    }
};

enum class BattleState{
    Entering,
    BeginTurn,
    TurnStart,
    WaitingForInput,
    SelectingSkill,
    SwitchingCharacter,
    ExecutingSkill,
    SwitchAnimating,
    CharacterFading,
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
    int switchOutIndex = 0;
};

struct BattleRewards {
    int coins = 0;
    int crystals = 0;
    int exp = 0;
};

struct BattleStats {
    int totalDamageDealt = 0;
    int totalDamageReceived = 0;
    int totalHealingDone = 0;
    int criticalHits = 0;
    int skillsUsed = 0;
    int turnsElapsed = 0;
    std::array<int, 3> playerDamageDealt = {0, 0, 0};
};

struct BattleContext{
    BattleTeam player, enemy;
    BattleSide currentTurn;
    BattleState state = BattleState::Entering;
    BattleAction pendingAction;
    int turnNumber = 1;
    BattleStats battleStats;
    BattleRewards rewards;
    Texture2D* background = nullptr;
    std::vector<std::string> defeatedEnemyNames;
    bool forfeit = false;
};

struct UILayout{
    Rectangle skillButton, characterSelector, switchCharacter, skipTurn, forfeit;
    bool skillButtonPressed, characterSelectorPressed, switchCharacterPressed, skipTurnPressed, forfeitPressed;
    Rectangle bottomBar;
    Rectangle skillLabels[4];
    Rectangle playerHudName, playerHudBar, playerHudText;
    Rectangle enemyHudName, enemyHudBar, enemyHudText;
    Rectangle playerPanel, enemyPanel;
    std::array<Rectangle, 3> playerSlots;
    std::array<Rectangle, 3> enemySlots;
};

class BattleUI{
    public:
        BattleUI();
        void Update(BattleContext& ctx, float dt);
        void Draw(const BattleContext& ctx) const;
        void StartTurnTransition(){ timer = 0.0f; }

    private:
        UILayout layout;
        float timer = 0.0f, IntroOut = 2.6f;
        float TurnStartDur = 0.7f;
        int selectedSkill = 0, selectedTarget = 0;
        bool showPlayerDependantUi = true;
        bool forfeitConfirmation = false;
        float skillButtonOffset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        void UpdateSkillButtonOffsets(const BattleContext& ctx, float dt);
        void DrawBottomBar(const BattleContext& ctx) const;
        void DrawTeamPanel(const BattleTeam& team, bool isPlayer, Vector2 origin) const;
        void DrawActiveHUD(const C& c, Vector2 position, const char* label, bool isPlayer) const;
        void DrawSkillButtons(const BattleContext& ctx) const;
        void DrawTurnIndicator(const BattleContext& ctx) const;
        void DrawEntering(int sW, int sH) const;
        void DrawTurnStart(int sW, int sH, int turnNumber) const;
        std::array<Rectangle, 3> GetSwitchPortraitRects(const BattleTeam& team) const;
};

struct AttackAnimation{
    enum class Kind { Attack, Buff };
    Kind kind = Kind::Attack;
    bool active = false;
    C *attacker = nullptr;
    C *target = nullptr;
    Vector2 attackerHome = {0, 0};
    Vector2 attackerOrigin = {0, 0};
    Vector2 targetPos = {0, 0};
    float lungeTime = 0.0f;
    float shakeTime = 0.0f;
    float damageTime = 0.0f;
    float buffTime = 0.0f;
    bool applied = false;
    int pendingDamage = 0;
    bool isCrit = false;
    static constexpr float LUNGE_DUR = 0.28f;
    static constexpr float SHAKE_DUR = 0.35f;
    static constexpr float DAMAGE_DUR = 1.1f;
    static constexpr float BUFF_DUR = 0.9f;
};

struct SwitchAnimation{
    bool active = false;
    int switchOutIndex = 0;
    int switchInIndex = 0;
    float progress = 0.0f;
    static constexpr float DURATION = 0.5f;
};

struct DeathAnimation{
    bool active = false;
    bool isPlayer = true;
    int deadIndex = 0;
    int nextIndex = -1;
    float fade = 0.0f;
    float slide = 0.0f;
    bool sliding = false;
    static constexpr float FADE_DUR = 0.6f;
    static constexpr float SLIDE_DUR = 0.4f;
};

class BattleRenderer{

    public:
        void Draw(const BattleContext& ctx, const AttackAnimation& attack, const SwitchAnimation& switchAnim, const DeathAnimation& deathAnim) const;
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
        void EndTurn();
        void FinishTurnTransition();
        void CheckDefeats();
        void UpdateAnimation(float dt);
        void AwardRewards();

    private:
        BattleContext battle;
        BattleUI ui;
        BattleRenderer renderer;
        AttackAnimation attack;
        SwitchAnimation switchAnim;
        DeathAnimation deathAnim;
        Texture2D* background = nullptr;
        bool transitioning = false;
        float transitionTimer = 0.0f;
        Music* battleMusic = nullptr;
    };

bool IntroAnimation(float dt);