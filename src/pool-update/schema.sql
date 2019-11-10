PRAGMA user_version=15; --keep in sync with pool.cpp

DROP TABLE IF EXISTS "units";
CREATE TABLE "units" (
	'uuid'	TEXT NOT NULL UNIQUE,
	'name'	TEXT NOT NULL,
	'manufacturer'	TEXT NOT NULL,
	'filename'	TEXT NOT NULL,
	'pool_uuid'	TEXT NOT NULL,
	'overridden'	BOOL NOT NULL,
	PRIMARY KEY('uuid')
);

DROP TABLE IF EXISTS "entities";
CREATE TABLE "entities" (
	'uuid'	TEXT NOT NULL UNIQUE,
	'name'	TEXT NOT NULL,
	'manufacturer'	TEXT NOT NULL,
	'n_gates'	INTEGER NOT NULL,
	'prefix'	TEXT NOT NULL,
	'filename'	TEXT NOT NULL,
	'pool_uuid'	TEXT NOT NULL,
	'overridden'	BOOL NOT NULL,
	PRIMARY KEY('uuid')
);

DROP TABLE IF EXISTS "symbols";
CREATE TABLE "symbols" (
	'uuid'	TEXT NOT NULL UNIQUE,
	'unit'	TEXT NOT NULL,
	'name'	TEXT NOT NULL,
	'filename'	TEXT NOT NULL,
	'pool_uuid'	TEXT NOT NULL,
	'overridden'	BOOL NOT NULL,
	PRIMARY KEY('uuid')
);

DROP TABLE IF EXISTS "packages";
CREATE TABLE "packages" (
	'uuid'	TEXT NOT NULL UNIQUE,
	'name'	TEXT NOT NULL,
	'manufacturer'	TEXT NOT NULL,
	'n_pads'	INTEGER NOT NULL,
	'alternate_for'	TEXT NOT NULL,
	'filename'	TEXT NOT NULL,
	'pool_uuid'	TEXT NOT NULL,
	'overridden'	BOOL NOT NULL,
	PRIMARY KEY('uuid')
);

DROP TABLE IF EXISTS "models";
CREATE TABLE "models" (
	'package_uuid'		TEXT NOT NULL,
	'model_uuid'		TEXT NOT NULL,
	'model_filename'	TEXT NOT NULL
);

DROP TABLE IF EXISTS "parts";
CREATE TABLE "parts" (
	'uuid'	TEXT NOT NULL UNIQUE,
	'MPN'	TEXT,
	'manufacturer'	TEXT,
	'entity'	TEXT NOT NULL,
	'package'	TEXT NOT NULL,
	'description'	TEXT NOT NULL,
	'parametric_table'	TEXT NOT NULL,
	'base'	TEXT NOT NULL,
	'filename'	TEXT,
	'pool_uuid'	TEXT NOT NULL,
	'overridden'	BOOL NOT NULL,
	PRIMARY KEY('uuid')
);

DROP INDEX IF EXISTS part_mpn;
CREATE INDEX part_mpn ON parts (MPN COLLATE naturalCompare ASC);

DROP INDEX IF EXISTS part_manufacturer;
CREATE INDEX part_manufacturer ON parts (manufacturer COLLATE naturalCompare ASC);

DROP TABLE IF EXISTS "orderable_MPNs";
CREATE TABLE "orderable_MPNs" (
	'part'	TEXT NOT NULL,
	'uuid'	TEXT NOT NULL,
	'MPN'	TEXT,
	PRIMARY KEY('part', 'uuid')
);

DROP TABLE IF EXISTS "tags";
CREATE TABLE 'tags' (
	'tag'	TEXT NOT NULL,
	'uuid'	TEXT NOT NULL,
	'type'	TEXT NOT NULL,
	PRIMARY KEY('tag','uuid','type')
);

DROP INDEX IF EXISTS tag_tag;
CREATE INDEX tag_tag ON tags (tag ASC);

DROP INDEX IF EXISTS tag_uuid;
CREATE INDEX tag_uuid ON tags (uuid ASC);

DROP TABLE IF EXISTS "padstacks";
CREATE TABLE "padstacks" (
	'uuid'	TEXT NOT NULL UNIQUE,
	'name'	TEXT NOT NULL,
	'well_known_name'	TEXT NOT NULL,
	'package'	TEXT NOT NULL,
	'filename'	TEXT NOT NULL,
	'type'	TEXT NOT NULL,
	'pool_uuid'	TEXT NOT NULL,
	'overridden'	BOOL NOT NULL,
	PRIMARY KEY('uuid')
);

DROP TABLE IF EXISTS "frames";
CREATE TABLE "frames" (
	'uuid'	TEXT NOT NULL UNIQUE,
	'name'	TEXT NOT NULL,
	'filename'	TEXT NOT NULL,
	'pool_uuid'	TEXT NOT NULL,
	'overridden'	BOOL NOT NULL,
	PRIMARY KEY('uuid')
);

DROP TABLE IF EXISTS "dependencies";
CREATE TABLE "dependencies" (
	'type'	TEXT NOT NULL,
	'uuid'	TEXT NOT NULL,
	'dep_type'	TEXT NOT NULL,
	'dep_uuid'	TEXT NOT NULL,
	UNIQUE('type', 'uuid', 'dep_type', 'dep_uuid') ON CONFLICT IGNORE
);

DROP VIEW IF EXISTS "all_items_view";
CREATE VIEW "all_items_view" AS
	SELECT 'unit' AS 'type', uuid AS uuid, filename AS filename, name AS name, pool_uuid AS pool_uuid FROM units UNION
	SELECT 'symbol' AS 'type', uuid AS uuid, filename AS filename, name AS name, pool_uuid AS pool_uuid FROM symbols UNION
	SELECT 'entity' AS 'type', uuid AS uuid, filename AS filename, name AS name, pool_uuid AS pool_uuid FROM entities UNION
	SELECT 'padstack' AS 'type', uuid AS uuid, filename AS filename, name AS name, pool_uuid AS pool_uuid FROM padstacks UNION
	SELECT 'package' AS 'type', uuid AS uuid, filename AS filename, name AS name, pool_uuid AS pool_uuid FROM packages UNION
	SELECT 'frame' AS 'type', uuid AS uuid, filename AS filename, name AS name, pool_uuid AS pool_uuid FROM frames UNION
	SELECT 'part' AS 'type', uuid AS uuid, filename AS filename, MPN AS name, pool_uuid AS pool_uuid FROM parts;

DROP VIEW IF EXISTS "tags_view";
CREATE VIEW "tags_view" AS
	SELECT type as type, uuid as uuid, GROUP_CONCAT(tag, ' ') as tags from tags group by tags.uuid, tags.type;
