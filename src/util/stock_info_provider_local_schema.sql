CREATE TABLE `stock_info` (
	`uuid`	BLOB PRIMARY KEY,
	`qty`	INTEGER NOT NULL,
	`price`	REAL NOT NULL default 0,
	`location`	TEXT,
	`last_updated`	DATETIME NOT NULL default CURRENT_TIMESTAMP
);
PRAGMA user_version=1;
