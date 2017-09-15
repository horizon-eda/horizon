CREATE TABLE IF NOT EXISTS "units" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);
CREATE TABLE IF NOT EXISTS "entities" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`n_gates`	INTEGER NOT NULL,
	`prefix`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);
CREATE TABLE IF NOT EXISTS "symbols" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`unit`	TEXT NOT NULL,
	`name`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);
CREATE TABLE IF NOT EXISTS "packages" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`n_pads`	INTEGER NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);
CREATE TABLE IF NOT EXISTS "parts" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`MPN`	TEXT,
	`manufacturer`	TEXT,
	`entity`	TEXT NOT NULL,
	`package`	TEXT NOT NULL,
	`filename`	TEXT,
	PRIMARY KEY(`uuid`)
);
CREATE TABLE `tags` (
	`tag`	TEXT NOT NULL,
	`uuid`	TEXT NOT NULL,
	`type`	TEXT NOT NULL,
	PRIMARY KEY(`tag`,`uuid`)
);
CREATE TABLE IF NOT EXISTS "padstacks" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`package`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	`type`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);
