#include "skillvm.h"
#include "battleScene.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <ctime>

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

void SkillVM::BeginAction(){
    critDecided = false;
    lastWasCrit = false;
}

void SkillVM::Prepare(Stats *actor, Stats *target, Skill *skill){
    actorStats = actor;
    targetStats = target;
    this->skill = skill;
    if(actorStats->maxHP <= 0) actorStats->maxHP = actorStats->HP;
    if(targetStats->maxHP <= 0) targetStats->maxHP = targetStats->HP;
    SkillConditions();
}

void SkillVM::SkillConditions(){
    conditions.clear();
    for(auto &mods : skill->modifiers){
        auto it = mods.additionalParams.find("condition");
        if(it != mods.additionalParams.end()){
            conditions.push_back(it->second.get<std::string>());
        }
    }
}

void SkillVM::Resolve(Stats *actor, Stats *target, Skill *skill, const std::vector<Skill> *actorSkills, bool applyDamage){
    Prepare(actor, target, skill);
    for(auto &mod: skill->modifiers){
        if(mod.type == "damage") DamageTypeSkill(mod, applyDamage);
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
            auto it = mod.additionalParams.find("condition");
            if(it == mod.additionalParams.end() || it->second.get<std::string>() != trigger) continue;
            SkillModifiers &m = const_cast<SkillModifiers&>(mod);
            if(m.additionalParams.find("duration") == m.additionalParams.end())
                m.additionalParams["duration"] = -1;
            if(m.type == "buff") BuffTypeSkill(m, passive.title);
            else if(m.type == "debuff") DebuffTypeSkill(m);
            else if(m.type == "heal") HealTypeSkill(m);
        }
    }
}

int SkillVM::DamageTypeSkill(SkillModifiers &mod, bool apply){
    float ATKScale = 0.0f, DEFScale = 0.0f, HPScale = 0.0f;
    bool hasScale = false;
    bool isCrit;
    if(critDecided){
        isCrit = lastWasCrit;
    }
    else{
        bool first = critCalc.isCrit(actorStats->CRIT_RATE);
        bool second = critCalc.isCrit(actorStats->CRIT_RATE);
        isCrit = first || second;
        lastWasCrit = isCrit;
        critDecided = true;
    }
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
    int damage = static_cast<int>(trueDamage);
    if(apply){
        targetStats->HP -= damage;
        if(targetStats->HP < 0) targetStats->HP = 0;
    }
    return damage;
}

int SkillVM::ComputeDamage(Stats *actor, Stats *target, Skill *skill){
    Prepare(actor, target, skill);
    int total = 0;
    for(auto &mod: skill->modifiers){
        if(mod.type == "damage") total += DamageTypeSkill(mod, false);
    }
    return total;
}

void SkillVM::BuffTypeSkill(SkillModifiers &mod, const std::string &label){
    auto it = mod.additionalParams.find("stat");
    if(it == mod.additionalParams.end()) return;
    std::string stat = it->second;
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
    if(duration != 0){
        actorStats->activeBuffs.push_back({stat, delta, duration, label});
    }
}

void SkillVM::HealTypeSkill(SkillModifiers &mod){
    auto it = mod.additionalParams.find("stat");
    if(it == mod.additionalParams.end()) return;
    if(it->second != "HP") return;
    float multiplier = mod.additionalParams["value"];
    int heal = static_cast<int>(targetStats->maxHP * multiplier);
    targetStats->HP = std::min(targetStats->HP + heal, targetStats->maxHP);
}

void SkillVM::DebuffTypeSkill(SkillModifiers &mod){
    auto it = mod.additionalParams.find("stat");
    if(it == mod.additionalParams.end()) return;
    std::string stat = it->second;
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

void SkillVM::CooldownAndDurationManager(BattleContext &ctx){
    TickTeam(ctx.player);
    TickTeam(ctx.enemy);
}

void SkillVM::TickTeam(BattleTeam &team){
    for(int i = 0; i < team.members.size(); ++i){
        C &member = team.members[i];
        for(auto &s : member.skills){
            if(s.cooldown > 0) s.cooldown--;
        }
        for(size_t b = 0; b < member.stats.activeBuffs.size(); ){
            ActiveBuff &buff = member.stats.activeBuffs[b];
            if(buff.duration < 0){
                b++;
                continue;
            }
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
        stats.HP = std::min(std::max(0, stats.HP - static_cast<int>(buff.delta)), stats.maxHP);
    }
    else if(buff.stat == "CRIT_RATE"){
        stats.CRIT_RATE = std::max(0.0f, stats.CRIT_RATE - buff.delta);
    }
    else if(buff.stat == "CRIT_DMG"){
        stats.CRIT_DMG = std::max(0.0f, stats.CRIT_DMG - buff.delta);
    }
}
