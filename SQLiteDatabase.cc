#include "SQLiteDatabase.h"

const QString SQLiteDatabase::DB_TYPE = "QSQLITE";

SQLiteDatabase::SQLiteDatabase() {
	// TODO Auto-generated constructor stub

}

SQLiteDatabase::~SQLiteDatabase() {
	// TODO Auto-generated destructor stub
}

void SQLiteDatabase::connect(const QString& dbFilename) {
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

QSqlDatabase SQLiteDatabase::open(const QString& file) {
	QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE, CONN_NAME);
	db.setHostName(DB_HOST);
	db.setDatabaseName(file);

	if (!db.open()) {
		qDebug() << "SQLiteDatabase: Unable to open a database file, error: " << db.lastError();
	}
	else {
		if (!hasCorrectCapabilities()) {
			qDebug() << "SQLiteDatabase: Did not have all the correct capabilities, certain methods may fail";
		}

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

void SQLiteDatabase::loadFromXML(const QString& XMLFile) {
	if (!isOpen()) {
		QFileInfo fi(XMLFile);

		connect(fi.absolutePath() + "/" + fi.baseName() + ".db");
	}

	SQLDatabase::loadFromXML(XMLFile);
}

void SQLiteDatabase::setPragmas() {
	database().exec("PRAGMA synchronous = OFF");
	database().exec("PRAGMA journal_mode = MEMORY");
}
