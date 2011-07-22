#include "SQLMySqlDatabase.h"

const QString SQLMySqlDatabase::DB_TYPE = "QMYSQL";

SQLMySqlDatabase::SQLMySqlDatabase(QObject *parent) : SQLDatabase(parent, DB_TYPE) {
	// TODO Auto-generated constructor stub

}

SQLMySqlDatabase::~SQLMySqlDatabase() {
	// TODO Auto-generated destructor stub
}

QString SQLMySqlDatabase::createViewQuery(const QString& viewName, const QString& selectStatement) const {
	return QString("CREATE VIEW %1 AS %2").arg(viewName).arg(selectStatement);
}

QSet<QString> SQLMySqlDatabase::tableFields(const QString& tableName) const {
	QSet<QString> fields;

	QSqlQuery query(database());

	if (query.exec(QString("DESCRIBE %1;").arg(tableName))) {
		QSqlRecord record = query.record();

		int name = record.indexOf("Field");

		while (query.next()) {
			fields << query.value(name).toString();
		}
	}
	else {
		qDebug() << "SQLMySqlDatabase: error on 'DESCRIBE':" << query.lastError();
	}

	qDebug() << "SQLMySqlDatabase::tableFields: returned" << fields;

	return fields;
}
