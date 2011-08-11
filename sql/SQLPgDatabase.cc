#include "SQLPgDatabase.h"

const QString SQLPgDatabase::DB_TYPE = "QPSQL";

SQLPgDatabase::SQLPgDatabase(QObject *parent) : SQLDatabase(parent, DB_TYPE) {
	// TODO Auto-generated constructor stub

}

SQLPgDatabase::~SQLPgDatabase() {
	// TODO Auto-generated destructor stub
}

QString SQLPgDatabase::createViewQuery(const QString& viewName, const QString& selectStatement) const {
	return QString("CREATE OR REPLACE VIEW %1 AS (%2);").arg(viewName).arg(selectStatement);
}

void SQLPgDatabase::setPragmas() { }

QSet<QString> SQLPgDatabase::tableFields(const QString& tableName) const {
	QSet<QString> fields;

	QSqlQuery query(database());

	if (query.exec(QString("SELECT column_name FROM information_schema.columns WHERE table_name = '%1';").arg(tableName))) {
		while (query.next()) {
			fields << query.value(0).toString();
		}
	}
	else {
		qDebug() << "SQLPgDatabase::: error fetching table info:" << query.lastError();
	}

	//qDebug() << "SQLMySqlDatabase::tableFields: returned" << fields;

	return fields;
}

QString SQLPgDatabase::schemaName() const {
	return "public";
}

void SQLPgDatabase::createHistory(const QString& table) {
	QSqlQuery query(database());

	transaction();

	if (query.exec(QString("CREATE TABLE %1_history AS SELECT * FROM %1 WHERE 1=2").arg(table))) {
		qDebug() << "SQLPgDatabase::createHistory: succesfully created history for" << table;
	}
	else {
		qDebug() << "SQLPgDatabase::createHistory: couldn't create history table for" << table << "->" << query.lastError() << "\n\tExecuted:" << query.lastQuery();
		database().rollback();
	}

	if (
		query.exec(QString("ALTER TABLE %1_history ADD COLUMN user_id INT").arg(table)) &&
		query.exec(QString("ALTER TABLE %1_history ADD COLUMN timestamp INT").arg(table))
	) {
		qDebug() << "SQLPgDatabase::createHistory: succesfully added history column for" << table;
	}
	else {
		qDebug() << "SQLPgDatabase::createHistory: couldn't add columns for" << table << "->" << query.lastError() << "\n\tExecuted:" << query.lastQuery();
		database().rollback();
	}

	commit();
}
