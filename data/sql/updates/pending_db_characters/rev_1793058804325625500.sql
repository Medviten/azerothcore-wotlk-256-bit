-- Retail race looks of the wowdecomp-256-bit client (src/server/shared/RetailLookWire.h): the full Retail customization
-- of a character whose race the client maps to a Retail model, 132 bytes (version, race, sex/form, option count,
-- 64 choice indices). The server stores and forwards it without interpreting it.
CREATE TABLE IF NOT EXISTS `character_retail_look` (
  `guid` INT UNSIGNED NOT NULL,
  `data` BINARY(132) NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Two Forms (Worgen Wolf/Human, Dracthyr Dragon/Visage): the look of the form a character is not showing, same layout.
-- The form showing keeps its look in character_retail_look.
CREATE TABLE IF NOT EXISTS `character_form_look` (
  `guid` INT UNSIGNED NOT NULL,
  `data` BINARY(132) NOT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
