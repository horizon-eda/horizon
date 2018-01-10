PRAGMA user_version=3;

DROP TABLE IF EXISTS "units";
CREATE TABLE "units" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);

DROP TABLE IF EXISTS "entities";
CREATE TABLE "entities" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`n_gates`	INTEGER NOT NULL,
	`prefix`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);

DROP TABLE IF EXISTS "symbols";
CREATE TABLE "symbols" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`unit`	TEXT NOT NULL,
	`name`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);

DROP TABLE IF EXISTS "packages";
CREATE TABLE "packages" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`manufacturer`	TEXT NOT NULL,
	`n_pads`	INTEGER NOT NULL,
	`alternate_for`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);

DROP TABLE IF EXISTS "parts";
CREATE TABLE "parts" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`MPN`	TEXT,
	`manufacturer`	TEXT,
	`entity`	TEXT NOT NULL,
	`package`	TEXT NOT NULL,
	`description`	TEXT NOT NULL,
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
CREATE TABLE "padstacks" (
	`uuid`	TEXT NOT NULL UNIQUE,
	`name`	TEXT NOT NULL,
	`package`	TEXT NOT NULL,
	`filename`	TEXT NOT NULL,
	`type`	TEXT NOT NULL,
	PRIMARY KEY(`uuid`)
);

DROP VIEW IF EXISTS "all_items_view";
CREATE VIEW "all_items_view" AS
	SELECT 'unit' AS 'type', uuid AS uuid, 'units/'||filename AS filename, name AS name FROM units UNION
	SELECT 'symbol' AS 'type', uuid AS uuid, 'symbols/'||filename AS filename, name AS name FROM symbols UNION
	SELECT 'entity' AS 'type', uuid AS uuid, 'entities/'||filename AS filename, name AS name FROM entities UNION
	SELECT 'padstack' AS 'type', uuid AS uuid, filename AS filename, name AS name FROM padstacks UNION
	SELECT 'package' AS 'type', uuid AS uuid, 'packages/'||filename AS filename, name AS name FROM packages UNION
	SELECT 'part' AS 'type', uuid AS uuid, 'parts/'||filename AS filename, MPN AS name FROM parts;
