#include "SQLDatabase.h"

#include <QtSql>

SQLDatabase::SQLDatabase(const QString& dbFile) {
	// check if the given file is a valid SQLite file

	// if not create an empty .db file
	setupDB(dbFile);
}

SQLDatabase::~SQLDatabase() {
	// TODO Auto-generated destructor stub

}

void SQLDatabase::loadFromXML(const QString& XMLFile) {

}

void SQLDatabase::resetDB() {
	if (mDb != NULL) {
		// TODO: clean house

		mDb->close();
	}
}

void SQLDatabase::setupDB(const QString& file) {
	resetDB();

	mDb = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(file);

	if (!db.open) {
		qDebug() << "Unable to open a database file for setup";
		return;
	}

	QSqlQuery query;
	query.exec("create table person (id int primary key, name varchar(20), address varchar(200), typeid int)");
}
