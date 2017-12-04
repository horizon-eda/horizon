PRAGMA user_version=1;

DROP TABLE IF EXISTS "units";
CREATE TABLE IF NOT EXISTS "units" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);

DROP TABLE IF EXISTS "entities";
CREATE TABLE IF NOT EXISTS "entities" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`n_gates`	INTEGER NOT NULL,
	`prefix`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);

DROP TABLE IF EXISTS "symbols";
CREATE TABLE IF NOT EXISTS "symbols" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`unit`	TEXT NOT NULL,
	`name`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);

DROP TABLE IF EXISTS "packages";
CREATE TABLE IF NOT EXISTS "packages" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`n_pads`	INTEGER NOT NULL,
	`alternate_for`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);

DROP TABLE IF EXISTS "parts";
CREATE TABLE IF NOT EXISTS "parts" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`MPN`	TEXT,
	`manufacturer`	TEXT,
	`entity`	TEXT NOT NULL,
	`package`	TEXT NOT NULL,
	`filename`	TEXT,
	PRIMARY KEY(`uuid`)
);

DROP TABLE IF EXISTS "tags";
CREATE TABLE `tags` (
	`tag`	TEXT NOT NULL,
	`uuid`	TEXT NOT NULL,
	`type`	TEXT NOT NULL,
	PRIMARY KEY(`tag`,`uuid`)
);

DROP TABLE IF EXISTS "padstacks";
CREATE TABLE IF NOT EXISTS "padstacks" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`package`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	`type`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);
