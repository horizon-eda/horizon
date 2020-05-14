PRAGMA user_version=1;

CREATE TABLE 'window_state' (
	'window'	TEXT NOT NULL UNIQUE,
	'width'	INTEGER NOT NULL,
	'height'	INTEGER NOT NULL,
	'x'	INTEGER NOT NULL,
	'y'	INTEGER NOT NULL,
	'maximized'	BOOLEAN NOT NULL,
	PRIMARY KEY('window')
);

CREATE TABLE 'implicit_prefs' (
	'key' TEXT NOT NULL UNIQUE,
	'vaue' TEXT NOT NULL,
	PRIMARY KEY('key')
);
