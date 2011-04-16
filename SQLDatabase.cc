#include "SQLDatabase.h"

#include <QtSql>
#include <QFile>
#include <QTextStream>

#include "XF.h"

using namespace thera;

const QString SQLDatabase::CONN_NAME = "TheraSQL";
const QString SQLDatabase::SCHEMA_FILE = "db/schema.sql";
const QString SQLDatabase::DB_TYPE = "QSQLITE";
const QString SQLDatabase::DB_HOST = "localhost";

const QString SQLDatabase::MATCHES_ROOTTAG = "matches";
const QString SQLDatabase::MATCHES_DOCTYPE = "matches-cache";
const QString SQLDatabase::OLD_MATCHES_VERSION = "0.0";
const QString SQLDatabase::MATCHES_VERSION = "1.0";

SQLDatabase::SQLDatabase() {
	/* nothing */
}

SQLDatabase::~SQLDatabase() {
	close();
}

bool SQLDatabase::isOpen() const {
	return database().isValid() && database().isOpen();
}

QSqlDatabase SQLDatabase::database() const {
	return QSqlDatabase::database(CONN_NAME);
}

void SQLDatabase::loadFile(const QString& dbFilename) {
	// if there is already a database opened, close it
	close();

	if (dbFilename != "") {
		QFile dbFile(dbFilename);

		// check if the given file is a valid SQLite file
		if (dbFile.exists()) {
			qDebug() << "Database file exists, attempting to load";

			open(dbFilename);

			if (isOpen()) {
				qDebug() << "Succesfully opened database, tables:" << database().tables();
			}
		}
		else {
			// if not create an empty .db file
			qDebug() << "No existing database found, creating new one";

			open(dbFilename);

			if (isOpen()) {
				setup(SCHEMA_FILE);

				qDebug() << "Setup new database, tables:" << database().tables();
			}
		}
	}
	else {
		qDebug("Did not receive valid database filename");
	}
}

void SQLDatabase::loadFromXML(const QString& XMLFile) {
	if (XMLFile == "") {
		qDebug("SQLDatabase::loadFromXML: filename was empty, aborting...");

		return;
	}

	QFile file(XMLFile);

	// open the file in read-only mode
	if (file.open(QIODevice::ReadOnly)) {
		// check if there is a database loaded, if not we'll make one on the fly and just use the same filename with .db extensions
		if (!isOpen()) {
			QFileInfo fi(file);

			loadFile(fi.absolutePath() + "/" + fi.baseName() + ".db");
		}

		QDomDocument doc;

		bool succes = doc.setContent(&file);

		file.close();

		if (succes) {
			QDomElement root(doc.documentElement());

			parseXML(root);
		}
		else {
			qDebug() << "Reading XML file" << XMLFile << "failed";
		}
	}
	else {
		qDebug() << "Could not open"  << XMLFile;
	}
}

void SQLDatabase::saveToXML(const QString& XMLFile) const {
	if (XMLFile == "") {
		qDebug("SQLDatabase::saveToXML: filename was empty, aborting...");

		return;
	}

	QFile file(XMLFile);

	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QTextStream out(&file);
		QDomDocument doc(toXML());

		doc.save(out, 1);
		file.close();
	}
	else {
		qDebug() << "SQLDatabase::saveToXML: Could not open"  << XMLFile;
	}
}

QDomDocument SQLDatabase::toXML() const {
	if (!isOpen()) {
		qDebug() << "Database wasn't open, couldn't convert to XML";

		return QDomDocument();
	}

	QDomDocument doc(MATCHES_DOCTYPE);
	QDomElement matches = doc.createElement(MATCHES_ROOTTAG);
	matches.setAttribute("version", MATCHES_VERSION);

	doc.appendChild(matches);

	return doc;
}

int SQLDatabase::matchCount() const {
	if (!isOpen()) {
		return 0;
	}

	QSqlDatabase db(database());
	QSqlQuery query(db);

	if (query.exec("SELECT Count(*) FROM matches") && query.first()) {
		return query.value(0).toInt();
	}
	else {
		return 0;
	}
}

/**
 * TODO: batch queries (make VariantList's)
 * TODO: more sanity-checking for corrupt matches.xml
 * TODO: decide if we use the preset matches.xml id, or a new one
 *
 * Schemas for quick reference:

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
	`match_id` INTEGER,
	`other_match_id` INTEGER
);
 */
void SQLDatabase::parseXML(const QDomElement &root) {
	register int i = 1;

	QSqlDatabase db(database());

	// check if the current database supports feature QSqlDriver::LastInsertId, we're going to use it
	if (!db.driver()->hasFeature(QSqlDriver::LastInsertId)) {
		qDebug() << "Database says it doesn't support LastInsertId, aborting";

		return;
	}

	if (!db.transaction()) {
		qDebug() << "Was unable to start transaction to fill tables";

		return;
	}

	// prepare queries
	QSqlQuery matchesQuery(db);
	matchesQuery.prepare(
		"INSERT INTO matches (match_id, source_id, source_name, target_id, target_name, transformation) "
		"VALUES (:match_id, :source_id, :source_name, :target_id, :target_name, :transformation)"
	);

	QSqlQuery conflictsQuery(db);
	conflictsQuery.prepare(
		"INSERT INTO conflicts (match_id, other_match_id) "
		"VALUES (:match_id, :other_match_id)"
	);

	QSqlQuery statusQuery(db);
	statusQuery.prepare(
		"INSERT INTO status (match_id, status) "
		"VALUES (:match_id, :status)"
	);

	QSqlQuery errorQuery(db);
	errorQuery.prepare(
		"INSERT INTO error (match_id, error) "
		"VALUES (:match_id, :error)"
	);

	QSqlQuery overlapQuery(db);
	overlapQuery.prepare(
		"INSERT INTO overlap (match_id, overlap) "
		"VALUES (:match_id, :overlap)"
	);

	QSqlQuery volumeQuery(db);
	volumeQuery.prepare(
		"INSERT INTO volume (match_id, volume) "
		"VALUES (:match_id, :volume)"
	);

	QSqlQuery old_volumeQuery(db);
	old_volumeQuery.prepare(
		"INSERT INTO old_volume (match_id, old_volume) "
		"VALUES (:match_id, :old_volume)"
	);

	emit databaseOpStarted(tr("Converting XML file to database"), root.childNodes().length());

	for (QDomElement match = root.firstChildElement("match"); !match.isNull(); match = match.nextSiblingElement()) {
		// update fragments table (we're going to go after fragments as we need them
		// probably an addFragment method is best
		// note that in reality this info should come from the old Database class, and not ad-hoc
		// as we are doing now
		// TODO: how can we check that this isn't already in the table? for now assume clean table

		int matchId = match.attribute("id").toInt();
		//int matchId = query.lastInsertId().toInt();

		QString debug = QString("item %1: source = %2, target = %3 || (id = %4)").arg(i).arg(match.attribute("src")).arg(match.attribute("tgt")).arg(matchId);
		//qDebug() << debug;

		// update matches table

		//XF transformation;
		QString rawTransformation(match.attribute("xf", "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1").toAscii()); // should be QTextStream if we want the real XF
		//rawTransformation >> transformation;

		matchesQuery.bindValue(":match_id", matchId);
		matchesQuery.bindValue(":source_id", 0); // TODO: not use dummy value
		matchesQuery.bindValue(":source_name", match.attribute("src"));
		matchesQuery.bindValue(":target_id", 0); // TODO: not use dummy value
		matchesQuery.bindValue(":target_name", match.attribute("tgt"));
		matchesQuery.bindValue(":transformation", rawTransformation);
		matchesQuery.exec();

		// update attribute tables
		statusQuery.bindValue(":match_id", matchId);
		statusQuery.bindValue(":status", match.attribute("status", "0").toInt());
		statusQuery.exec();

		errorQuery.bindValue(":match_id", matchId);
		errorQuery.bindValue(":error", match.attribute("error", "NaN").toDouble());
		errorQuery.exec();

		overlapQuery.bindValue(":match_id", matchId);
		overlapQuery.bindValue(":overlap", match.attribute("overlap", "0.0").toDouble());
		overlapQuery.exec();

		volumeQuery.bindValue(":match_id", matchId);
		volumeQuery.bindValue(":volume", match.attribute("volume", "0.0").toDouble());
		volumeQuery.exec();

		old_volumeQuery.bindValue(":match_id", matchId);
		old_volumeQuery.bindValue(":old_volume", match.attribute("old_volume", "0.0").toDouble());
		old_volumeQuery.exec();

		// update conflicts table
		QString conflictString = match.attribute("conflict", "");
		QStringList conflicts = conflictString.split(" ");

		foreach (const QString &c, conflicts) {
			/*
			query.prepare(
				"INSERT INTO conflicts (match_id, other_match_id) "
				"VALUES (:match_id, :other_match_id)"
			);
			*/
			conflictsQuery.bindValue(":match_id", matchId);
			conflictsQuery.bindValue(":other_match_id", c.toInt());
			conflictsQuery.exec();
		}

		emit databaseOpStepDone(i);

		++i;
	}

	if (!db.commit()) {
		qDebug() << "Was unable to commit transaction to fill tables";
	}

	emit databaseOpEnded();
	emit matchCountChanged();
}

void SQLDatabase::reset() {
	// disconnect and unlink db file + call setup
}

void SQLDatabase::setup(const QString& schemaFile) {
	// get database
	QSqlDatabase db = database();

	// read schema file
	QFile file(schemaFile);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Schema file '" << schemaFile << "' could not be opened, aborting";

		return;
	}

	QByteArray data(file.readAll());
	file.close();

	QString schemaQuery = QString(data);

	//start transatcion
	if (!db.transaction()) {
		qDebug() << "Couldn't start transaction";

		return;
	}

	QStringList queries = schemaQuery.split(";");
	QSqlQuery query(db);

	foreach (const QString &q, queries) {
		query.exec(q);
		qDebug() << "Executed query:" << q;
	}

	if (!db.commit()) {
		qDebug() << "Could not commit to database even though transaction was started" << db.lastError();
	}
}

QSqlDatabase SQLDatabase::open(const QString& file) {
	QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE, CONN_NAME);
	db.setHostName(DB_HOST);
	db.setDatabaseName(file);

	if (!db.open()) {
		qDebug() << "Unable to open a database file for setup, error: " << db.lastError();
	}
	else {
		if (db.isValid()) {
			/* a tiny bit of performance tuning */
			setPragmas();

			emit databaseOpened();
			emit matchCountChanged();
		}
		else {
			qDebug() << "Somehow database was opened but it wasn't valid: " << db.lastError();
		}
	}

	return db;
}

void SQLDatabase::setPragmas() {
	database().exec("PRAGMA synchronous = OFF");
	database().exec("PRAGMA journal_mode = MEMORY");
}

void SQLDatabase::close() {
	if (isOpen()) {
		database().close();

		emit databaseClosed();
	}
}
