-- 256-bit race masks for CONDITION_RACE (16): ConditionValue1-4 are the four 64-bit words of the race mask
-- (races 1-64, 65-128, 129-192, 193-256), so `ConditionValue1 = @SomeRaceMask` keeps working for races up to 64.
-- All other condition types still use 32-bit values in ConditionValue1-3 and leave ConditionValue4 at 0.
ALTER TABLE `conditions`
    MODIFY COLUMN `ConditionValue1` BIGINT UNSIGNED NOT NULL DEFAULT '0',
    MODIFY COLUMN `ConditionValue2` BIGINT UNSIGNED NOT NULL DEFAULT '0',
    MODIFY COLUMN `ConditionValue3` BIGINT UNSIGNED NOT NULL DEFAULT '0',
    ADD COLUMN `ConditionValue4` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `ConditionValue3`,
    DROP PRIMARY KEY,
    ADD PRIMARY KEY (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `ConditionValue4`);
