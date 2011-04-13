#include "SQLDatabase.h"

#include <QtSql>
#include <QFile>

#include "qglobal.h"

const QString SQLDatabase::CONN_NAME = "TheraSQL";
const QString SQLDatabase::SCHEMA_FILE = "db/schema.sql";
const QString SQLDatabase::DB_TYPE = "QSQLITE";
const QString SQLDatabase::DB_HOST = "localhost";

const QString SQLDatabase::MATCHES_ROOTTAG = "matches";

SQLDatabase::SQLDatabase(const QString& dbFilename) {
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

SQLDatabase::~SQLDatabase() {
	QSqlDatabase db = database();

	db.close();
}

bool SQLDatabase::isOpen() const {
	return database().isOpen();
}

QSqlDatabase SQLDatabase::database() const {
	return QSqlDatabase::database(CONN_NAME);
}

void SQLDatabase::loadFromXML(const QString& XMLFile) {
	QFile file(XMLFile);

	if (file.open(QIODevice::ReadOnly)) {
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

/**
 * TODO: batch queries (make VariantList's)
 * TODO: more sanity-checking for corrupt matches.xml
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
 */
void SQLDatabase::parseXML(const QDomElement &root) {
	register int i = 1;

	QSqlDatabase db = database();
	QSqlQuery query(db);

	// check if the current database supports feature QSqlDriver::LastInsertId, we're going to use it
	if (!db.driver()->hasFeature(QSqlDriver::LastInsertId)) {
		qDebug() << "Database says it doesn't support LastInsertId, aborting";

		return;
	}

	if (!db.transaction()) {
		qDebug() << "Was unable to start transaction to fill tables";

		return;
	}

	for (QDomElement match = root.firstChildElement("match"); !match.isNull(); match = match.nextSiblingElement()) {
		QString debug = QString("item %1: source = %2, target = %3").arg(i).arg(match.attribute("src")).arg(match.attribute("tgt"));
		qDebug() << debug;

		// update fragments table (we're going to go after fragments as we need them
		// probably an addFragment method is best
		// note that in reality this info should come from the old Database class, and not ad-hoc
		// as we are doing now
		// TODO: how can we check that this isn't already in the table? for now assume clean table


		// update matchinfo table
		query.prepare(
			"INSERT INTO matchinfo (id, status, overlap, error, volume, old_volume) "
			"VALUES (NULL, :status, :overlap, :error, :volume, :old_volume)"
		);
		query.bindValue(":status", match.attribute("status").toInt());
		query.bindValue(":overlap", match.attribute("overlap").toDouble());
		query.bindValue(":error", match.attribute("error").toDouble());
		query.bindValue(":volume", match.attribute("volume").toDouble());
		query.bindValue(":old_volume", match.attribute("old_volume").toDouble());
		query.exec();

		int matchId = query.lastInsertId().toInt();

		query.finish();

		// update matches table
		query.prepare(
			"INSERT INTO matches (match_id, fragment_id, transformation) "
			"VALUES (:match_id, :fragment_id, :transformation)"
		);
		query.bindValue(":match_id", matchId);
		query.bindValue(":fragment_id", 0); // TODO: not use dummy value
		query.bindValue(":transformation", 0);
		query.exec();

		// update conflicts table

		++i;
	}

	if (!db.commit()) {
		qDebug() << "Was unable to commit transaction to fill tables";
	}

}

void SQLDatabase::reset() {
	// disconnect and unlink db file + call setup
}

void SQLDatabase::setup(const QString& schemaFile) {
	QSqlDatabase db = database();

	QString schemaQuery = readSqlFile(schemaFile);

	if (db.transaction()) {
		//qDebug() << "Preparing to execute query:\n" << schemaQuery;

		QStringList queries = schemaQuery.split(";");

		QSqlQuery query(db);
		foreach (const QString &q, queries) {
			query.exec(q);
			qDebug() << "Executed query:" << q;
		}

		//query.exec("CREATE TABLE matches (id int primary key, name varchar(20), address varchar(200), typeid int)");

		if (!db.commit()) {
			qDebug() << "Could not commit to database even though transaction was started" << db.lastError();
		}
		else {
			qDebug() << "Successfully created database, tables:\n" << db.tables();
		}
	}
	else {
		qDebug() << "Was unable to start transaction to build database: " << db.lastError();
	}
}

QSqlDatabase SQLDatabase::open(const QString& file) {
	QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE, CONN_NAME);
	db.setHostName(DB_HOST);
	db.setDatabaseName(file);

	if (!db.open()) {
		qDebug() << "Unable to open a database file for setup, error: " << db.lastError();
	}

	return db;
}

const QString SQLDatabase::readSqlFile(const QString& schemaFilename) const {
	QFile file(schemaFilename);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Schema file '" << schemaFilename << "' could not be opened, aborting";

		return QString();
	}

	QByteArray data(file.readAll());

	file.close();

	return QString(data);
}



