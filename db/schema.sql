CREATE TABLE `fragments` (
	`id` INTEGER PRIMARY KEY,
	`name` TEXT
);
CREATE TABLE `matchinfo` (
	`id` INTEGER PRIMARY KEY,
	`status` INTEGER,
	`overlap` REAL,
	`error` REAL,
	`volume` REAL,
	`old_volume` REAL
);
CREATE TABLE `matches` (
	`match_id` INTEGER,
	`fragment_id` INTEGER,
	`transformation` BLOB
);
CREATE TABLE `conflicts` (
	`id` INTEGER,
	`other_id` INTEGER
);