-- 256-bit race masks: four 64-bit words per mask. The original column keeps races 1-64, so SQL bitwise math on it
-- keeps working; <column>_2, _3 and _4 hold races 65-128, 129-192 and 193-256.
-- MODIFY also converts the VARCHAR columns written by an earlier version of this update (decimal text).
-- item_template.AllowableRace stays signed: -1 still means all races.
ALTER TABLE `item_template`
    MODIFY COLUMN `AllowableRace` BIGINT NOT NULL DEFAULT '-1',
    ADD COLUMN `AllowableRace_2` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `AllowableRace`,
    ADD COLUMN `AllowableRace_3` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `AllowableRace_2`,
    ADD COLUMN `AllowableRace_4` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `AllowableRace_3`;

ALTER TABLE `quest_template`
    MODIFY COLUMN `AllowableRaces` BIGINT UNSIGNED NOT NULL DEFAULT '0',
    ADD COLUMN `AllowableRaces_2` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `AllowableRaces`,
    ADD COLUMN `AllowableRaces_3` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `AllowableRaces_2`,
    ADD COLUMN `AllowableRaces_4` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `AllowableRaces_3`;

ALTER TABLE `spell_area`
    MODIFY COLUMN `racemask` BIGINT UNSIGNED NOT NULL DEFAULT '0',
    ADD COLUMN `racemask_2` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `racemask`,
    ADD COLUMN `racemask_3` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `racemask_2`,
    ADD COLUMN `racemask_4` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `racemask_3`,
    DROP PRIMARY KEY,
    ADD PRIMARY KEY (`spell`, `area`, `quest_start`, `aura_spell`, `racemask`, `racemask_2`, `racemask_3`, `racemask_4`, `gender`);

ALTER TABLE `mail_level_reward`
    MODIFY COLUMN `raceMask` BIGINT UNSIGNED NOT NULL DEFAULT '0',
    ADD COLUMN `raceMask_2` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `raceMask`,
    ADD COLUMN `raceMask_3` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `raceMask_2`,
    ADD COLUMN `raceMask_4` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `raceMask_3`,
    DROP PRIMARY KEY,
    ADD PRIMARY KEY (`level`, `raceMask`, `raceMask_2`, `raceMask_3`, `raceMask_4`);

ALTER TABLE `playercreateinfo_skills`
    MODIFY COLUMN `raceMask` BIGINT UNSIGNED NOT NULL,
    ADD COLUMN `raceMask_2` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `raceMask`,
    ADD COLUMN `raceMask_3` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `raceMask_2`,
    ADD COLUMN `raceMask_4` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `raceMask_3`,
    DROP PRIMARY KEY,
    ADD PRIMARY KEY (`raceMask`, `raceMask_2`, `raceMask_3`, `raceMask_4`, `classMask`, `skill`);

ALTER TABLE `playercreateinfo_spell_custom`
    MODIFY COLUMN `racemask` BIGINT UNSIGNED NOT NULL DEFAULT '0',
    ADD COLUMN `racemask_2` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `racemask`,
    ADD COLUMN `racemask_3` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `racemask_2`,
    ADD COLUMN `racemask_4` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `racemask_3`,
    DROP PRIMARY KEY,
    ADD PRIMARY KEY (`racemask`, `racemask_2`, `racemask_3`, `racemask_4`, `classmask`, `Spell`);

ALTER TABLE `playercreateinfo_cast_spell`
    MODIFY COLUMN `raceMask` BIGINT UNSIGNED NOT NULL DEFAULT '0',
    ADD COLUMN `raceMask_2` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `raceMask`,
    ADD COLUMN `raceMask_3` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `raceMask_2`,
    ADD COLUMN `raceMask_4` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `raceMask_3`;

-- DBC override tables hold one column per DBC field: a widened race mask is 8 uint32 fields, so 7 more columns
-- follow each race mask column (SELECT * order must match the DBC layout).
ALTER TABLE `faction_dbc`
    ADD COLUMN `ReputationRaceMask_1_2` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_1`,
    ADD COLUMN `ReputationRaceMask_1_3` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_1_2`,
    ADD COLUMN `ReputationRaceMask_1_4` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_1_3`,
    ADD COLUMN `ReputationRaceMask_1_5` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_1_4`,
    ADD COLUMN `ReputationRaceMask_1_6` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_1_5`,
    ADD COLUMN `ReputationRaceMask_1_7` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_1_6`,
    ADD COLUMN `ReputationRaceMask_1_8` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_1_7`,
    ADD COLUMN `ReputationRaceMask_2_2` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_2`,
    ADD COLUMN `ReputationRaceMask_2_3` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_2_2`,
    ADD COLUMN `ReputationRaceMask_2_4` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_2_3`,
    ADD COLUMN `ReputationRaceMask_2_5` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_2_4`,
    ADD COLUMN `ReputationRaceMask_2_6` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_2_5`,
    ADD COLUMN `ReputationRaceMask_2_7` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_2_6`,
    ADD COLUMN `ReputationRaceMask_2_8` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_2_7`,
    ADD COLUMN `ReputationRaceMask_3_2` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_3`,
    ADD COLUMN `ReputationRaceMask_3_3` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_3_2`,
    ADD COLUMN `ReputationRaceMask_3_4` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_3_3`,
    ADD COLUMN `ReputationRaceMask_3_5` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_3_4`,
    ADD COLUMN `ReputationRaceMask_3_6` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_3_5`,
    ADD COLUMN `ReputationRaceMask_3_7` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_3_6`,
    ADD COLUMN `ReputationRaceMask_3_8` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_3_7`,
    ADD COLUMN `ReputationRaceMask_4_2` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_4`,
    ADD COLUMN `ReputationRaceMask_4_3` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_4_2`,
    ADD COLUMN `ReputationRaceMask_4_4` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_4_3`,
    ADD COLUMN `ReputationRaceMask_4_5` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_4_4`,
    ADD COLUMN `ReputationRaceMask_4_6` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_4_5`,
    ADD COLUMN `ReputationRaceMask_4_7` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_4_6`,
    ADD COLUMN `ReputationRaceMask_4_8` INT NOT NULL DEFAULT '0' AFTER `ReputationRaceMask_4_7`;

ALTER TABLE `skilllineability_dbc`
    ADD COLUMN `RaceMask_2` INT NOT NULL DEFAULT '0' AFTER `RaceMask`,
    ADD COLUMN `RaceMask_3` INT NOT NULL DEFAULT '0' AFTER `RaceMask_2`,
    ADD COLUMN `RaceMask_4` INT NOT NULL DEFAULT '0' AFTER `RaceMask_3`,
    ADD COLUMN `RaceMask_5` INT NOT NULL DEFAULT '0' AFTER `RaceMask_4`,
    ADD COLUMN `RaceMask_6` INT NOT NULL DEFAULT '0' AFTER `RaceMask_5`,
    ADD COLUMN `RaceMask_7` INT NOT NULL DEFAULT '0' AFTER `RaceMask_6`,
    ADD COLUMN `RaceMask_8` INT NOT NULL DEFAULT '0' AFTER `RaceMask_7`;

ALTER TABLE `skillraceclassinfo_dbc`
    ADD COLUMN `RaceMask_2` INT NOT NULL DEFAULT '0' AFTER `RaceMask`,
    ADD COLUMN `RaceMask_3` INT NOT NULL DEFAULT '0' AFTER `RaceMask_2`,
    ADD COLUMN `RaceMask_4` INT NOT NULL DEFAULT '0' AFTER `RaceMask_3`,
    ADD COLUMN `RaceMask_5` INT NOT NULL DEFAULT '0' AFTER `RaceMask_4`,
    ADD COLUMN `RaceMask_6` INT NOT NULL DEFAULT '0' AFTER `RaceMask_5`,
    ADD COLUMN `RaceMask_7` INT NOT NULL DEFAULT '0' AFTER `RaceMask_6`,
    ADD COLUMN `RaceMask_8` INT NOT NULL DEFAULT '0' AFTER `RaceMask_7`;

ALTER TABLE `talenttab_dbc`
    ADD COLUMN `RaceMask_2` INT NOT NULL DEFAULT '0' AFTER `RaceMask`,
    ADD COLUMN `RaceMask_3` INT NOT NULL DEFAULT '0' AFTER `RaceMask_2`,
    ADD COLUMN `RaceMask_4` INT NOT NULL DEFAULT '0' AFTER `RaceMask_3`,
    ADD COLUMN `RaceMask_5` INT NOT NULL DEFAULT '0' AFTER `RaceMask_4`,
    ADD COLUMN `RaceMask_6` INT NOT NULL DEFAULT '0' AFTER `RaceMask_5`,
    ADD COLUMN `RaceMask_7` INT NOT NULL DEFAULT '0' AFTER `RaceMask_6`,
    ADD COLUMN `RaceMask_8` INT NOT NULL DEFAULT '0' AFTER `RaceMask_7`;
