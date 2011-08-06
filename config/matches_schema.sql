CREATE TABLE `users` (
	`user_id` INTEGER PRIMARY KEY,
	`name` TEXT,
	`surname` TEXT,
	`mail` TEXT DEFAULT '',
);
CREATE TABLE `matches` (
	`match_id` INTEGER PRIMARY KEY,
	`source_name` TEXT,
	`target_name` TEXT,
	`transformation` TEXT
);