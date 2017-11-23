CREATE TABLE `window_state` (
	`window`	TEXT NOT NULL UNIQUE,
	`width`	INTEGER NOT NULL,
	`height`	INTEGER NOT NULL,
	`x`	INTEGER NOT NULL,
	`y`	INTEGER NOT NULL,
	`maximized`	BOOLEAN NOT NULL,
	PRIMARY KEY(`window`)
);
PRAGMA user_version=1;
