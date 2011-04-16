CREATE TABLE `fragments` (
	`id` INTEGER PRIMARY KEY,
	`name` TEXT
);
-- with the 'matches' table we can support multiple fragment matches , we might need to install triggers to ensure correctness of the 'transformation' attribute
-- NOTE: we might need to look out for NULL characters in the blob...
-- NOTE: maybe we should add AUTOINCREMENT to the id field. If not, it's possible id's are reused if/when we delete a match: http://www.sqlite.org/faq.html#q1 
CREATE TABLE `matches` (
	`match_id` INTEGER PRIMARY KEY, -- NOTE: NOT "INT", INTEGER increases performance twofold on SQLite: http://www.sqlite.org/lang_createtable.html
	`source_id` INTEGER,
	`source_name` TEXT,
	`target_id` INTEGER,
	`target_name` TEXT,
	`transformation` TEXT -- for now text seems easier, although it might be worse than BLOB, we'll look into it later
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

/*
Possible ways of implementing enums, would also need UPDATE trigger)

CREATE TRIGGER EnumTrg 
	BEFORE INSERT ON MainTbl 
	FOR EACH ROW 
	WHEN (SELECT 1 FROM EnumVals WHERE val = new.EnumCol LIMIT 1) IS NULL 
	BEGIN 
		SELECT raise(rollback, 'forign-key violation: MainTbl.EnumCol'); 
	END;
CREATE TRIGGER EnumTrg 
	BEFORE INSERT ON MainTbl 
	FOR EACH ROW
	WHEN (SELECT count(*) FROM EnumVals WHERE val = new.EnumCol) = 0
	BEGIN
		SELECT raise(rollback, 'foreign-key violation: MainTbl.EnumCol'); 
	END;	
*/