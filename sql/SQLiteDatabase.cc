#include "SQLiteDatabase.h"

const QString SQLiteDatabase::DB_TYPE = "QSQLITE";

SQLiteDatabase::SQLiteDatabase(QObject *parent) : SQLDatabase(parent, DB_TYPE) {
	// TODO Auto-generated constructor stub
}

SQLiteDatabase::~SQLiteDatabase() {
	// TODO Auto-generated destructor stub
}

/*
void SQLiteDatabase::connect(const QString& dbFilename) {
	if (dbFilename != "") {
		// if there is already a database opened, close it
		close();

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
*/

/*
QSqlDatabase SQLiteDatabase::open(const QString& file) {
	QSqlDatabase db = QSqlDatabase::addDatabase(DB_TYPE, mConnectionName);
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
			// performance...
			setPragmas();

			emit databaseOpened();
			emit matchFieldsChanged(); // the order is actually important, because for example the models react to matchCountChanged, but matchFieldsChanged needs to have ran by then
			emit matchCountChanged();
		}
		else {
			qDebug() << "Somehow database was opened but it wasn't valid: " << db.lastError();
		}
	}

	return db;
}
*/

/*
void SQLiteDatabase::loadFromXML(const QString& XMLFile) {
	if (!isOpen()) {
		QFileInfo fi(XMLFile);

		connect(fi.absolutePath() + "/" + fi.baseName() + ".db");
	}

	SQLDatabase::loadFromXML(XMLFile);
}
*/

QStringList SQLiteDatabase::tables(QSql::TableType type) const {
	return database().tables(type);
}

QString SQLiteDatabase::createViewQuery(const QString& viewName, const QString& selectStatement) const {
	return QString("CREATE VIEW IF NOT EXISTS %1 AS %2").arg(viewName).arg(selectStatement);
}

void SQLiteDatabase::setPragmas() {
	QSqlQuery query(database());

	if (!query.exec("PRAGMA synchronous = OFF")) qDebug() << "SQLiteDatabase::setPragmas: setting pragma" << query.lastQuery() << "failed";
	if (!query.exec("PRAGMA journal_mode = MEMORY")) qDebug() << "SQLiteDatabase::setPragmas: setting pragma" << query.lastQuery() << "failed";
}

QSet<QString> SQLiteDatabase::tableFields(const QString& tableName) const {
	QSet<QString> fields;

	QSqlQuery query(database());

	if (query.exec(QString("PRAGMA TABLE_INFO(%1)").arg(tableName))) {
		QSqlRecord record = query.record();

		int name = record.indexOf("name");

		while (query.next()) {
			fields << query.value(name).toString();
		}
	}
	else {
		qDebug() << "SQLiteDatabase::tableFields: error on 'PRAGMA TABLE_INFO':" << query.lastError();
	}

	return fields;
}
