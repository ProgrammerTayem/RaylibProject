#include "skillvm.h"
#include "battleScene.h"
#include "nlohmann/json.hpp"

float CritCalculation::getCritForThisTurn(const float critRate){
    N_ofTurns++;
    auto it = PRDTable.lower_bound(critRate);
    if(it == PRDTable.begin()){
        return it->second;
    }
    auto prev_it = std::prev(it);

    float x1 = prev_it->first, y1 = prev_it->second;
    float x2 = it->first, y2 = it->second;
    float t = (critRate - x1) / (x2 - x1);
    return lerp(y1, y2, t);
}

bool CritCalculation::isCrit(const float critRate){
    float critChance = getCritForThisTurn(critRate);
    float probability = std::min(1.0f, static_cast<float>(N_ofTurns) * critChance);
    float randomValue = static_cast<float>(GetRandomValue(0, 100)) / 100.0f; 
    if(randomValue < probability) {
        N_ofTurns = 0; 
        return true;
    }
    return false;
}

void SkillVM::Resolve(Stats *actor, Stats *target, Skill *skill, const std::vector<Skill> *actorSkills){
    actorStats = actor;
    targetStats = target;
    this->skill = skill;
    if(actorStats->maxHP <= 0) actorStats->maxHP = actorStats->HP;
    if(targetStats->maxHP <= 0) targetStats->maxHP = targetStats->HP;
    SkillConditions();
    for(auto &mod: skill->modifiers){
        if(mod.type == "damage") DamageTypeSkill(mod);
        else if(mod.type == "buff") BuffTypeSkill(mod);
        else if(mod.type == "debuff") DebuffTypeSkill(mod);
        else if(mod.type == "heal") HealTypeSkill(mod);
    }
    if(actorSkills && lastWasCrit){
        for(auto &mod: skill->modifiers){
            if(mod.type == "damage") ApplyPassives(actor, target, *actorSkills, "CRIT_TRIG");
        }
    }
}

void SkillVM::ApplyPassives(Stats *actor, Stats *opponent, const std::vector<Skill> &skills, const std::string &trigger){
    actorStats = actor;
    targetStats = opponent;
    for(const Skill &passive : skills){
        if(passive.type != "PASSIVE") continue;
        for(const SkillModifiers &mod : passive.modifiers){
            bool matches = false;
            auto it = mod.additionalParams.find("condition");
            if(it != mod.additionalParams.end() && it->second.get<std::string>() == trigger) matches = true;
            if(!matches) continue;
            if(mod.type == "buff") BuffTypeSkill(const_cast<SkillModifiers&>(mod));
            else if(mod.type == "debuff") DebuffTypeSkill(const_cast<SkillModifiers&>(mod));
            else if(mod.type == "heal") HealTypeSkill(const_cast<SkillModifiers&>(mod));
        }
    }
}

void SkillVM::DamageTypeSkill(SkillModifiers &mod){
    float ATKScale = 0.0f, DEFScale = 0.0f, HPScale = 0.0f;
    bool hasScale = false;
    bool isCrit = critCalc.isCrit(actorStats->CRIT_RATE);
    lastWasCrit = isCrit;
    if(mod.additionalParams.find("scaling") != mod.additionalParams.end()){
        std::string scaleType = mod.additionalParams["scaling"];
        hasScale = true;
        if(scaleType == "ATK")
            ATKScale = mod.additionalParams["multiplier"];
        else if(scaleType == "DEF") DEFScale = mod.additionalParams["multiplier"];
        else if(scaleType == "HP") HPScale = mod.additionalParams["multiplier"];
    }

    int baseDamage = hasScale ? 0 : actorStats->ATK;
    for(auto &cond: conditions){
        if(cond == "ACTOR_HP_BELOW"){
            if(mod.additionalParams.find("condition_value") != mod.additionalParams.end() &&
               mod.additionalParams.find("condition_multiplier") != mod.additionalParams.end()){
                float when = mod.additionalParams["condition_value"];
                float multiplier = mod.additionalParams["condition_multiplier"];
                if(static_cast<float>(actorStats->HP) / 100.0f < when){
                    baseDamage = static_cast<int>(baseDamage * multiplier);
                }
            }
        }
    }

    float critDMG = isCrit ? (actorStats->CRIT_DMG - 1.0f) * (actorStats->ATK + ATKScale * actorStats->ATK) : 0.0f;
    float totalRealDamage = static_cast<float>(baseDamage)
        + (ATKScale * actorStats->ATK)
        + (DEFScale * targetStats->DEF * 100.0f)
        + (HPScale * static_cast<float>(actorStats->HP));
    float trueDamage = std::max(0.0f, totalRealDamage * (1.0f - targetStats->DEF));
    trueDamage += critDMG * (1.0f - targetStats->DEF);
    targetStats->HP -= static_cast<int>(trueDamage);
    if(targetStats->HP < 0) targetStats->HP = 0;
}

void SkillVM::BuffTypeSkill(SkillModifiers &mod){
    if(mod.additionalParams.find("stat") != mod.additionalParams.end()){
        std::string stat = mod.additionalParams["stat"];
        float multiplier = mod.additionalParams["value"];
        float delta = 0.0f;
        int duration = (mod.additionalParams.find("duration") != mod.additionalParams.end())
            ? static_cast<int>(mod.additionalParams["duration"]) : 0;
        if(stat == "ATK"){
            delta = actorStats->ATK * multiplier;
            actorStats->ATK += static_cast<int>(delta);
        }
        else if(stat == "DEF"){
            delta = actorStats->DEF * multiplier;
            actorStats->DEF += delta;
        }
        else if(stat == "HP"){
            delta = actorStats->HP * multiplier;
            actorStats->HP += static_cast<int>(delta);
            if(actorStats->HP > actorStats->maxHP) actorStats->HP = actorStats->maxHP;
        }
        else if(stat == "CRIT_RATE"){
            delta = multiplier;
            actorStats->CRIT_RATE += delta;
        }
        else if(stat == "CRIT_DMG"){
            delta = multiplier;
            actorStats->CRIT_DMG += delta;
        }
        if(duration > 0){
            actorStats->activeBuffs.push_back({stat, delta, duration});
        }
    }
}

void SkillVM::HealTypeSkill(SkillModifiers &mod){
    if(mod.additionalParams.find("stat") != mod.additionalParams.end()){
        std::string stat = mod.additionalParams["stat"];
        float multiplier = mod.additionalParams["value"];
        if(stat == "HP"){
            int heal = static_cast<int>(targetStats->maxHP * multiplier);
            targetStats->HP += heal;
            if(targetStats->HP > targetStats->maxHP) targetStats->HP = targetStats->maxHP;
        }
    }
}

void SkillVM::DebuffTypeSkill(SkillModifiers &mod){
    if(mod.additionalParams.find("stat") != mod.additionalParams.end()){
        std::string stat = mod.additionalParams["stat"];
        float multiplier = mod.additionalParams["value"];
        float delta = 0.0f;
        int duration = (mod.additionalParams.find("duration") != mod.additionalParams.end())
            ? static_cast<int>(mod.additionalParams["duration"]) : 0;
        if(stat == "ATK"){
            delta = targetStats->ATK * multiplier;
            targetStats->ATK = std::max(0, targetStats->ATK - static_cast<int>(delta));
        }
        else if(stat == "DEF"){
            delta = targetStats->DEF * multiplier;
            targetStats->DEF = std::max(0.0f, targetStats->DEF - delta);
        }
        else if(stat == "CRIT_RATE"){
            delta = multiplier;
            targetStats->CRIT_RATE = std::max(0.0f, targetStats->CRIT_RATE - delta);
        }
        else if(stat == "CRIT_DMG"){
            delta = multiplier;
            targetStats->CRIT_DMG = std::max(0.0f, targetStats->CRIT_DMG - delta);
        }
        if(duration > 0){
            targetStats->activeBuffs.push_back({stat, -delta, duration});
        }
    }
}

void SkillVM::CooldownAndDurationManager(BattleContext &ctx){
    TickTeam(ctx.player);
    TickTeam(ctx.enemy);
}

void SkillVM::TickTeam(BattleTeam &team){
    for(int i = 0; i < team.members.size(); ++i){
        C &member = team.members[i];
        for(int j = 0; j < member.skills.size(); ++j){
            if(member.skills[j].cooldown > 0) member.skills[j].cooldown--;
        }
        for(size_t b = 0; b < member.stats.activeBuffs.size(); ){
            ActiveBuff &buff = member.stats.activeBuffs[b];
            buff.duration--;
            if(buff.duration <= 0){
                RevertBuff(member.stats, buff);
                member.stats.activeBuffs.erase(member.stats.activeBuffs.begin() + b);
            }
            else b++;
        }
    }
}

void SkillVM::RevertBuff(Stats &stats, const ActiveBuff &buff){
    if(buff.stat == "ATK"){
        stats.ATK = std::max(0, stats.ATK - static_cast<int>(buff.delta));
    }
    else if(buff.stat == "DEF"){
        stats.DEF = std::max(0.0f, stats.DEF - buff.delta);
    }
    else if(buff.stat == "HP"){
        stats.HP = std::max(0, stats.HP - static_cast<int>(buff.delta));
        if(stats.HP > stats.maxHP) stats.HP = stats.maxHP;
    }
    else if(buff.stat == "CRIT_RATE"){
        stats.CRIT_RATE = std::max(0.0f, stats.CRIT_RATE - buff.delta);
    }
    else if(buff.stat == "CRIT_DMG"){
        stats.CRIT_DMG = std::max(0.0f, stats.CRIT_DMG - buff.delta);
    }
}
