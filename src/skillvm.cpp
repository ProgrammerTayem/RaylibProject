#include "skillvm.h"
#include "battleScene.h"
#include "nlohmann/json.hpp"

float CritCalculation::getCritForThisTurn(const float critRate){
    N_ofTurns++;
    auto it = PRDTable.lower_bound(critRate);
    auto prev_it = std::prev(it);

    float x1 = prev_it->first, y1 = prev_it->second;
    float x2 = it->first, y2 = it->second;
    float t = (critRate - x1) / (x2 - x1);
    return lerp(y1, y2, t);
}

bool CritCalculation::isCrit(const float critRate){
    float critChance = getCritForThisTurn(critRate);
    float probability = static_cast<float>(N_ofTurns) * critChance;
    float randomValue = static_cast<float>(GetRandomValue(0, 100)) / 100.0f; 
    if(randomValue < probability) {
        N_ofTurns = 0; 
        return true;
    }
    return false;
}

void SkillVM::Resolve(Stats *actor, Stats *target, Skill *skill){
    actorStats = actor;
    targetStats = target;
    this->skill = skill;
    SkillConditions();
    for(auto &mod: skill->modifiers){
        if(mod.type == "damage") DamageTypeSkill(mod);
        else if(mod.type == "buff") BuffTypeSkill(mod);
        else if(mod.type == "debuff") DebuffTypeSkill(mod);
        else if(mod.type == "heal") HealTypeSkill(mod);
    }
}

void SkillVM::DamageTypeSkill(SkillModifiers &mod){
    int baseDamage = actorStats->ATK;
    float ATKScale = 0.0f, DEFScale = 0.0f, HPScale = 0.0f;
    bool isCrit = critCalc.isCrit(actorStats->CRIT_RATE);
    if(mod.additionalParams.find("scale") != mod.additionalParams.end()){
        std::string scaleType = mod.additionalParams["scale"];
        if(scaleType == "ATK")
            ATKScale = mod.additionalParams["multiplier"];
        else if(scaleType == "DEF") DEFScale = mod.additionalParams["multiplier"];
        else if(scaleType == "HP") HPScale = mod.additionalParams["multiplier"];
    }
    for(auto &cond: conditions){
        if(cond == "ACTOR_HP_BELOW"){
            float when = mod.additionalParams["condition_value"];
            float multiplier = mod.additionalParams["condition_multiplier"];
            if(static_cast<float>(actorStats->HP) / 100.0f < when){
                baseDamage = static_cast<int>(baseDamage * multiplier);
            }
        }
    }
    float critDMG = isCrit ? actorStats->CRIT_DMG*actorStats->ATK : 0.0f;
    float totalRealDamage = baseDamage + critDMG + (ATKScale * actorStats->ATK) + (DEFScale * targetStats->DEF * 100) + (HPScale * targetStats->HP);
    float trueDamage = std::max(0.0f, totalRealDamage * (1.0f - targetStats->DEF));
    targetStats->HP -= static_cast<int>(trueDamage);
}

void SkillVM::BuffTypeSkill(SkillModifiers &mod){
    if(mod.additionalParams.find("stat") != mod.additionalParams.end()){
        std::string stat = mod.additionalParams["stat"];
        float multiplier = mod.additionalParams["value"];
        float duration = (mod.additionalParams["duration"]);
        if(stat == "ATK") actorStats->ATK += static_cast<int>(actorStats->ATK * multiplier);
        else if(stat == "DEF") actorStats->DEF += actorStats->DEF * multiplier;
        else if(stat == "HP") actorStats->HP += static_cast<int>(actorStats->HP * multiplier);
        else if(stat == "CRIT_RATE") actorStats->CRIT_RATE += multiplier;
        else if(stat == "CRIT_DMG") actorStats->CRIT_DMG += multiplier;
    }
}

void SkillVM::HealTypeSkill(SkillModifiers &mod){

}

void SkillVM::DebuffTypeSkill(SkillModifiers &mod){
    
}

void SkillVM::CooldownAndDurationManager(BattleContext &ctx){
    BattleTeam &cur = (ctx.currentTurn == BattleSide::Player) ? ctx.player : ctx.enemy;
    for(int i = 0; i < cur.members.size(); ++i){
        for(int j = 0; j < cur.members[i].skills.size(); ++j){
            if(cur.members[i].skills[j].cooldown > 0) cur.members[i].skills[j].cooldown--;
            else cur.members[i].skills[j].cooldown = ctx.copyPlayer.members[i].skills[j].cooldown;

            if(cur.members[i].skills[j].modifiers.empty()) continue;
            Skill skl = cur.members[i].skills[j];
            for(int k = 0; k < skl.modifiers.size(); k++){
                if(skl.modifiers[k].type == "buff" || skl.modifiers[k].type == "debuff"){
                    if(skl.modifiers[k].additionalParams.find("duration") != skl.modifiers[k].additionalParams.end()){
                        float duration = skl.modifiers[k].additionalParams["duration"];
                        if(duration > 0) duration--;
                        else skl = ctx.copyPlayer.members[i].skills[j];
                    }
                }
            }
        }
    }
    // for(int i = 0; i < ctx.enemy.members.size(); ++i){
    //     for(int j = 0; j < ctx.enemy.members[i].skills.size(); ++j){
    //         if(ctx.enemy.members[i].skills[j].cooldown > 0) ctx.enemy.members[i].skills[j].cooldown--;
    //         else ctx.enemy.members[i].skills[j].cooldown = ctx.copyEnemy.members[i].skills[j].cooldown;
    //     }
    //     if(ctx.enemy.members[i].skills[j].modifiers.empty()) continue;
    //     Skill skl = ctx.enemy.members[i].skills[j];
    //     for(int k = 0; k < skl.modifiers.size(); k++){
    //         if(skl.modifiers[k].type == "buff" || skl.modifiers[k].type == "debuff"){
    //             if(skl.modifiers[k].additionalParams.find("duration") != skl.modifiers[k].additionalParams.end()){
    //                 float duration = std::stof(skl.modifiers[k].additionalParams["duration"]);
    //                 if(duration > 0) duration--;
    //                 else skl = ctx.copyEnemy.members[i].skills[j];
    //             }
    //         }
    //     }
    // }
}