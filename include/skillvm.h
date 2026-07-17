#pragma once
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "gameData.h"

struct BattleContext;
struct BattleTeam;
struct ActiveBuff;

struct CritCalculation{
    std::map<float, double> PRDTable = {
        {0.0f, 0.0},
        {0.05f, 0.00380165},
        {0.1f, 0.014745},
        {0.15f, 0.032225},
        {0.2f, 0.05625},
        {0.25f, 0.086025},
        {0.3f, 0.12165},
        {0.35f, 0.162225},
        {0.4f, 0.20775},
        {0.45f, 0.257225},
        {0.5f, 0.31065},
        {0.55f, 0.368025},
        {0.6f, 0.42935},
        {0.65f, 0.494625},
        {0.7f, 0.56385},
        {0.75f, 0.637025},
        {0.8f, 0.71415},
        {0.85f, 0.795225},
        {0.9f, 0.88025},
        {0.95f, 0.949225},
    };
    int N_ofTurns = 0;

    float lerp(float a, float b, float t){
        return a + (b - a) * t;
    }
    float getCritForThisTurn(const float critRate);
    bool isCrit(const float critRate);
};


struct SkillVM{
    Stats *actorStats, *targetStats;
    Skill *skill;
    bool lastWasCrit = false;
    std::vector<std::string> conditions;
    CritCalculation critCalc;
    void SkillConditions(){
        conditions.clear();
        if(skill->modifiers.empty()) return;
        for(auto &mods : skill->modifiers){
            auto it = mods.additionalParams.find("condition");
            if(it != mods.additionalParams.end()){
                conditions.push_back(it->second.get<std::string>());
            }
        }
    }
    void Resolve(Stats *actor, Stats *target, Skill *skill, const std::vector<Skill> *actorSkills = nullptr);
    void DamageTypeSkill(SkillModifiers &mod);
    void BuffTypeSkill(SkillModifiers &mod);
    void DebuffTypeSkill(SkillModifiers &mod);
    void HealTypeSkill(SkillModifiers &mod);
    void ApplyPassives(Stats *actor, Stats *opponent, const std::vector<Skill> &skills, const std::string &trigger);
    void CooldownAndDurationManager(BattleContext &ctx);
    void TickTeam(BattleTeam &team);
    void RevertBuff(Stats &stats, const ActiveBuff &buff);
};