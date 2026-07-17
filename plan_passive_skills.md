# Plan: Passive-Skill Handling in Battle

## Goal
1. Remove PASSIVE skills from the player's selectable attack selection in `battleScene`.
2. Devise/implement the logic that actually handles passive skills (currently defined in each character's `.json` `skills` array with `"type": "PASSIVE"`). For the existing data, the only passive is `CRIT_TRIG`: "increase ATK by 0.5% each time a crit is triggered by self".

## Current state (relevant facts)
- Every character JSON (`shadow`, `phantom`, `jason`, `alice`) has `skills[2]` with `"type": "PASSIVE"`, `cooldown: -1`, `target: "self"`, effect `{type:"buff", stat:"ATK", value:0.005, condition:"CRIT_TRIG"}`.
- `Skill.type` is already parsed in `Loader.cpp:702` (`skill.type = skillJson.at("type")`).
- `battleScene.cpp` player input (`WaitingForInput`, lines 41-53) and `Draw` (lines 127-134) iterate all 4 skills, so the passive is shown as a usable button and CAN be selected.
- `SkillVM::Resolve` / `BuffTypeSkill` apply buffs immediately when a skill resolves; there is no passive/condition-driven trigger path and no concept of "this skill is passive".
- `SkillVM::SkillConditions()` collects `condition` strings (e.g. `"CRIT_TRIG"`) but is only consulted for `ACTOR_HP_BELOW` in `DamageTypeSkill`.

## Part A — Remove passive from selection (battleScene.cpp)
1. Add a helper in `BattleUI` (or `battleScene`): `bool IsPassive(const Skill&)` returning `skill.type == "PASSIVE"`.
2. In `BattleUI::Update` `WaitingForInput` player branch (lines 41-53): skip indices whose skill is passive — do not register `skillButtonPressed`, do not set `selectedSkill`. This prevents the passive from ever becoming `selectedSkill`.
3. In `BattleUI::Draw` `WaitingForInput`/`SelectingSkill` skill-button loop (lines 127-134): skip drawing the passive skill button (or render it dimmed/non-interactive). Skipping matches "remove from selection".
4. Enemy AI branch (lines 62-70) uses priority `{3,1,0}` which already excludes index 2 (the passive), so no change needed there — but guard it with `IsPassive` check for safety/future-proofing.

## Part B — Passive handling logic (SkillVM)
Design: passives are NOT activated by the player; they are applied automatically by the battle loop based on their `condition`.

1. **Represent a passive trigger.** Keep passives in `C::skills` (already loaded). Add a method on `SkillVM`:
   `void ApplyPassives(Stats* actor, Stats* opponent, const std::string& trigger)` that scans a character's skills for `type == "PASSIVE"` whose effect `condition == trigger`, and applies each effect.
2. **Trigger point — crit.** `DamageTypeSkill` already computes `isCrit` (line 48). When `isCrit` is true, after applying damage, invoke the actor's passives with trigger `"CRIT_TRIG"`:
   `ApplyPassives(actorStats, targetStats, "CRIT_TRIG");`
   (Need access to the actor's *full* skill list, not just the active `skill`; so pass the active `C` or its `skills` into `Resolve`/`DamageTypeSkill`, or have `BattleScene` call `ApplyPassives` directly using `battle.pendingAction.actor`.)
3. **Implement `ApplyPassives`** reusing `BuffTypeSkill`-style stat mutation but WITHOUT a `duration` (permanent for the battle, e.g. stacking +0.5% ATK each crit). For `CRIT_TRIG`/buff/ATK, replicate the ATK-delta math from `BuffTypeSkill` and push to `actor->activeBuffs` with `duration = 0` (or a dedicated permanent marker) so it is not reverted by `TickTeam` (which only reverts `duration > 0`).
4. **Wire the call site.** In `BattleScene::ProcessEffects` (after `svm.Resolve(...)`) or inside `svm.Resolve`, call the passive application for the actor. Cleanest: extend `SkillVM::Resolve` to receive the actor `C` (or store `actorSkills` alongside `actorStats`) so the crit trigger can reference all of the actor's passives.

## Proposed signature changes
- `SkillVM::Resolve(Stats* actor, Stats* target, Skill* skill)` → also keep `actorSkills` reference (e.g. add `const std::vector<Skill>* actorSkills` param) so passives can be read.
- Add `SkillVM::ApplyPassives(Stats* actor, Stats* opponent, const std::vector<Skill>& skills, const std::string& trigger)`.

## Files to change
- `src/battleScene.cpp` — Part A (skip passive in input + draw), Part B (call ApplyPassives on crit).
- `include/battleScene.h` — optionally declare `IsPassive`/selection helper.
- `src/skillvm.cpp` — add `ApplyPassives`; call it from crit path in `DamageTypeSkill`/`Resolve`.
- `include/skillvm.h` — declare `ApplyPassives`; update `Resolve` signature if needed.
- `src/Loader.cpp` — no change required (`type` already parsed); `condition` already captured via `additionalParams`.

## Verification
- Build with CMake (`cmake --build`).
- In-battle: passive skill button no longer appears/selectable for the player; selecting skills 0/1/3 works.
- Landing a critical hit increases the actor's ATK by 0.5% per `CRIT_TRIG` passive, repeatedly stacking; non-crit hits do not.
- Enemy AI never attempts to "use" the passive (priority already excludes it).
