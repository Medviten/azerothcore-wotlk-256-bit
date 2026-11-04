CREATE TABLE IF NOT EXISTS `player_race_template` (
  `race` tinyint unsigned NOT NULL,
  `home_race` tinyint unsigned NOT NULL COMMENT 'Stock race whose start area this race shares: taxi nodes, Collector''s Edition voucher',
  `kin_race` tinyint unsigned NOT NULL COMMENT 'Race whose looks and language this race shares: holiday costumes, race change language. 0 for none',
  `shapeshift_skin_color` tinyint unsigned NOT NULL DEFAULT 0 COMMENT 'Shapeshift forms follow skin (fur) color instead of hair color',
  PRIMARY KEY (`race`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='What custom races copy from the stock races. A race without a row is its own home and kin';

REPLACE INTO `player_race_template` (`race`, `home_race`, `kin_race`, `shapeshift_skin_color`) VALUES
(6, 6, 6, 1); -- Tauren
