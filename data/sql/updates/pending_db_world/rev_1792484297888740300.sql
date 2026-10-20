-- Unlimited reputation spillover targets: one row per (faction, spillover faction), loaded after
-- `reputation_spillover_template` (whose 6 slots are not enough with many custom races). A row overrides that
-- faction's template slot for the same spillover faction; a faction with any spillover row no longer uses the
-- DBC parent faction spillover, exactly like a template row.
CREATE TABLE IF NOT EXISTS `reputation_spillover_faction` (
    `faction` INT UNSIGNED NOT NULL COMMENT 'faction entry (Faction.dbc) gaining the reputation',
    `spillover_faction` INT UNSIGNED NOT NULL COMMENT 'faction entry (Faction.dbc) that gets spillover',
    `rate` FLOAT NOT NULL DEFAULT '0' COMMENT 'the given rep points * rate',
    `max_rank` TINYINT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'max rank, above this spillover_faction gets no spillover',
    PRIMARY KEY (`faction`, `spillover_faction`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='Reputation spillover targets beyond reputation_spillover_template';
