-- 256-bit race masks: store them as strings (decimal, -1 for all races, or 0x-prefixed hex)
-- Existing integer values are kept as their decimal text, which the core still accepts.
ALTER TABLE `item_template` MODIFY COLUMN `AllowableRace` VARCHAR(66) NOT NULL DEFAULT '-1';
ALTER TABLE `quest_template` MODIFY COLUMN `AllowableRaces` VARCHAR(66) NOT NULL DEFAULT '0';
ALTER TABLE `spell_area` MODIFY COLUMN `racemask` VARCHAR(66) NOT NULL DEFAULT '0';
ALTER TABLE `mail_level_reward` MODIFY COLUMN `raceMask` VARCHAR(66) NOT NULL DEFAULT '0';
ALTER TABLE `playercreateinfo_skills` MODIFY COLUMN `raceMask` VARCHAR(66) NOT NULL;
ALTER TABLE `playercreateinfo_spell_custom` MODIFY COLUMN `racemask` VARCHAR(66) NOT NULL DEFAULT '0';
ALTER TABLE `playercreateinfo_cast_spell` MODIFY COLUMN `raceMask` VARCHAR(66) NOT NULL DEFAULT '0';
