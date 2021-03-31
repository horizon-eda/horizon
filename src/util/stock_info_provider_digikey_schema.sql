CREATE TABLE `cache` (
	`MPN`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`data`	TEXT NOT NULL,
	`last_updated`	DATETIME NOT NULL,
	PRIMARY KEY (`MPN`, `manufacturer`)
);

CREATE TABLE `tokens` (
	`key`	TEXT NOT NULL,
	`value`	TEXT NOT NULL,
    `valid_until`	DATETIME NOT NULL,
	PRIMARY KEY (`key`)
);
PRAGMA user_version=1;

