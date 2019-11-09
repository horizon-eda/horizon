CREATE TABLE `cache` (
	`MPN`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`data`	TEXT NOT NULL,
	`last_updated`	DATETIME NOT NULL,
	PRIMARY KEY (`MPN`, `manufacturer`)
);
PRAGMA user_version=1;
