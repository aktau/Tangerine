#include "SQLiteDatabase.h"

const QString SQLiteDatabase::DB_TYPE = "QSQLITE";

SQLiteDatabase::SQLiteDatabase(QObject *parent) : SQLDatabase(parent, DB_TYPE) {

}

SQLiteDatabase::~SQLiteDatabase() {

}

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

void SQLiteDatabase::createHistory(const QString& table) {
	QSqlQuery query(database());

	transaction();

	if (query.exec(QString("CREATE TABLE %1_history AS SELECT * FROM %1 WHERE 1=2").arg(table))) {
		qDebug() << "SQLiteDatabase::createHistory: succesfully created history for" << table;
	}
	else {
		qDebug() << "SQLiteDatabase::createHistory: couldn't create history table for" << table << "->" << query.lastError() << "\n\tExecuted:" << query.lastQuery();
		database().rollback();
	}

	if (
		query.exec(QString("ALTER TABLE %1_history ADD COLUMN user_id INT").arg(table)) &&
		query.exec(QString("ALTER TABLE %1_history ADD COLUMN timestamp INT").arg(table))
	) {
		qDebug() << "SQLiteDatabase::createHistory: succesfully added history column for" << table;
	}
	else {
		qDebug() << "SQLiteDatabase::createHistory: couldn't add columns for" << table << "->" << query.lastError() << "\n\tExecuted:" << query.lastQuery();
		database().rollback();
	}

	commit();
}

// temporary place to hide backup code that will need to be implemented
//http://www.qtcentre.org/threads/36131-Attempting-to-use-Sqlite-backup-api-from-driver-handle-fails

/*
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QString>
#include <QVariant>
#include "sqlite3/sqlite3.h"

void backup(QString src, QString dst) {
	QSqlDatabase sqlDb = QSqlDatabase::addDatabase("QSQLITE");
	sqlDb.setDatabaseName(src);
	sqlDb.open();

	QVariant v = sqlDb.driver()->handle();
	sqlite3* pSource = *static_cast<sqlite3 **>(v.data());

	int rc;
	sqlite3 *pDest;
	sqlite3_backup *pBackup;
	rc = sqlite3_open(dst.toLocal8Bit().data(), &pDest);
	if(rc == SQLITE_OK) {
		pBackup = sqlite3_backup_init(pDest, "main", pSource, "main");
		if (pBackup) {
			do {
				rc = sqlite3_backup_step(pBackup, 5);
				if (rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
					sqlite3_sleep(250);
				}
			} while(rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED );

			//Release resources allocated by backup_init().
			sqlite3_backup_finish(pBackup);
		}
		sqlite3_close(pDest);
	}

	sqlDb.close();
}


int main(int argc, char *argv[]) {
	backup("/home/arkay/Projects/QtWebApp/database/Current/architektur.db",
		   "/home/arkay/Projects/QtWebApp/database/Backup/architektur.db");
}
*/
