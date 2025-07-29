CREATE TABLE `kingdoms` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(255) NOT NULL,
  `flag` varchar(255) NOT NULL,
  `map_data` text NOT NULL,
  `player_id` int(11) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `player_id` (`player_id`),
  CONSTRAINT `kingdoms_ibfk_1` FOREIGN KEY (`player_id`) REFERENCES `player` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

ALTER TABLE `player` ADD COLUMN `kingdom_id` INT(11) UNSIGNED DEFAULT NULL;
ALTER TABLE `player` ADD CONSTRAINT `player_ibfk_1` FOREIGN KEY (`kingdom_id`) REFERENCES `kingdoms` (`id`) ON DELETE SET NULL;
