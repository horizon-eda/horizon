CREATE TABLE IF NOT EXISTS "resistors" (
	`part`	TEXT NOT NULL UNIQUE,
	`value`	REAL,
	`pmax`	REAL,
	`tolerance`	REAL,
	PRIMARY KEY(`part`)
);
CREATE TABLE `capacitors` (
	`part`	TEXT NOT NULL UNIQUE,
	`value`	REAL,
	`wvdc`	REAL,
	`characteristic`	TEXT,
	PRIMARY KEY(`part`)
);
