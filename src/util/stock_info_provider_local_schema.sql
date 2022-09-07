CREATE TABLE `stock_info` (
	`uuid`	TEXT NOT NULL,
	`qty`	INTEGER NOT NULL,
	`price`	REAL NOT NULL default 0,
	`location`	TEXT,
	`last_updated`	DATETIME NOT NULL default CURRENT_TIMESTAMP,
	PRIMARY KEY (`uuid`)
);
PRAGMA user_version=1;
