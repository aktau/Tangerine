#include "SQLDatabase.h"

#include <QtSql>
#include <QFile>

const QString SQLDatabase::CONN_NAME = "TheraSQL";
const QString SQLDatabase::DB_TYPE = "QSQLITE";
const QString SQLDatabase::DB_HOST = "localhost";

SQLDatabase::SQLDatabase(const QString& dbFilename) {
	QFile dbFile(dbFilename);

	// check if the given file is a valid SQLite file
	if (dbFile.exists()) {
		qDebug() << "Database file exists, attempting to load";

		// initialize
	}
	else {
		// if not create an empty .db file
		qDebug() << "No existing file found, creating new one";

		setupDB(dbFilename);
	}
}

SQLDatabase::~SQLDatabase() {
	/*
	if (mDb != NULL) {
		mDb->close();
	}
	*/
}

void SQLDatabase::loadFromXML(const QString& XMLFile) {

}

void SQLDatabase::resetDB() {
	/*
	if (mDb != NULL) {
		// TODO: clean house

		mDb->close();
	}
	*/
}

void SQLDatabase::setupDB(const QString& file) {
	resetDB();

	qDebug() << "Attempting to make a DB file";

	QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE, CONN_NAME);
	db.setHostName(DB_HOST);
	db.setDatabaseName(file);

	qDebug() << db.tables();

	if (!db.open()) {
		qDebug() << "Unable to open a database file for setup";
		return;
	}

	QSqlQuery query(db);
	query.exec("create table person (id int primary key, name varchar(20), address varchar(200), typeid int)");

	db.close();
}
