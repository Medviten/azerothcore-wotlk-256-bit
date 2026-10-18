-- 256-bit race masks: 'Race' conditions hold races 1-64 in conditionValue and races 65-256 in conditionValue_2-4.
-- MODIFY also converts the VARCHAR column written by an earlier version of this update (decimal text).
ALTER TABLE `mail_server_template_conditions`
    MODIFY COLUMN `conditionValue` BIGINT UNSIGNED NOT NULL,
    ADD COLUMN `conditionValue_2` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `conditionValue`,
    ADD COLUMN `conditionValue_3` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `conditionValue_2`,
    ADD COLUMN `conditionValue_4` BIGINT UNSIGNED NOT NULL DEFAULT '0' AFTER `conditionValue_3`;
