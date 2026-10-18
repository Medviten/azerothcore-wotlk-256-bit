-- 256-bit race masks: 'Race' conditions store a race mask string (decimal, -1 or 0x-prefixed hex), all other types a number
ALTER TABLE `mail_server_template_conditions` MODIFY COLUMN `conditionValue` VARCHAR(66) NOT NULL;
