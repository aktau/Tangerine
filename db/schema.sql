CREATE TABLE `fragments` (
	`id` INTEGER PRIMARY KEY,
	`name` TEXT
);
CREATE TABLE `matches` (
	`match_id` INTEGER PRIMARY KEY,
	`source_id` INTEGER,
	`source_name` TEXT,
	`target_id` INTEGER,
	`target_name` TEXT,
	`transformation` TEXT
);
CREATE TABLE `conflicts` (
	`match_id` INTEGER,
	`other_match_id` INTEGER
);
CREATE TABLE `status` (
	`match_id` INTEGER PRIMARY KEY,
	`status` INTEGER
);
CREATE TABLE `error` (
	`match_id` INTEGER PRIMARY KEY,
	`error` REAL
);
CREATE TABLE `overlap` (
	`match_id` INTEGER PRIMARY KEY,
	`overlap` REAL
);
CREATE TABLE `volume` (
	`match_id` INTEGER PRIMARY KEY,
	`volume` REAL
);
CREATE TABLE `old_volume` (
	`match_id` INTEGER PRIMARY KEY,
	`old_volume` REAL
);
CREATE TABLE `comments` (
	`match_id` INTEGER PRIMARY KEY,
	`comments` TEXT
);