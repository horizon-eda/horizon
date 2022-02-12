PRAGMA user_version=3;
PRAGMA journal_mode=WAL;

CREATE TABLE IF NOT EXISTS 'window_state' (
	'window'	TEXT NOT NULL UNIQUE,
	'width'	INTEGER NOT NULL,
	'height'	INTEGER NOT NULL,
	'x'	INTEGER NOT NULL,
	'y'	INTEGER NOT NULL,
	'maximized'	BOOLEAN NOT NULL,
	PRIMARY KEY('window')
);

CREATE TABLE IF NOT EXISTS 'widths' (
	'key' TEXT NOT NULL UNIQUE,
	'width' INTEGER NOT NULL,
	PRIMARY KEY('key')
);

CREATE TABLE IF NOT EXISTS 'plane_update_dialog_visibility' (
	'board' TEXT NOT NULL,
	'plane' TEXT NOT NULL,
	'visible' INTEGER NOT NULL,
	PRIMARY KEY('board', 'plane')
);
