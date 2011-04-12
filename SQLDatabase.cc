#include "SQLDatabase.h"

#include <QtSql>
#include <QFile>

#include "qglobal.h"

const QString SQLDatabase::CONN_NAME = "TheraSQL";
const QString SQLDatabase::SCHEMA_FILE = "db/schema.sql";
const QString SQLDatabase::DB_TYPE = "QSQLITE";
const QString SQLDatabase::DB_HOST = "localhost";

SQLDatabase::SQLDatabase(const QString& dbFilename) {
	QFile dbFile(dbFilename);

	// check if the given file is a valid SQLite file
	if (dbFile.exists()) {
		qDebug() << "Database file exists, attempting to load";

		load(dbFilename);
	}
	else {
		// if not create an empty .db file
		qDebug() << "No existing file found, creating new one";

		setup(dbFilename, SCHEMA_FILE);
	}
}

SQLDatabase::~SQLDatabase() {
	QSqlDatabase db = QSqlDatabase::database(CONN_NAME);

	db.close();
}

void SQLDatabase::loadFromXML(const QString& XMLFile) {

}

void SQLDatabase::reset() {
	// disconnect and unlink db file + call setup
}

void SQLDatabase::setup(const QString& databaseFile, const QString& schemaFile) {
	QSqlDatabase db = open(databaseFile);

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

void SQLDatabase::load(const QString& file) {
	QSqlDatabase db = open(file);

	if (db.isValid()) {
		qDebug() << "Tables:" << db.tables();
	}
	else {
		qDebug() << "Invalid DB file encountered";
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



