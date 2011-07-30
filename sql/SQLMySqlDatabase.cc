#include "SQLMySqlDatabase.h"

const QString SQLMySqlDatabase::DB_TYPE = "QMYSQL";

SQLMySqlDatabase::SQLMySqlDatabase(QObject *parent) : SQLDatabase(parent, DB_TYPE) { }
SQLMySqlDatabase::~SQLMySqlDatabase() { }

QString SQLMySqlDatabase::makeCompatible(const QString& statement) const {
	// look for double pipes/'||' and turn them into CONCAT statements
	QString newStatement = statement;
	QRegExp rx("(?:(\\w*)\\s*\\|\\|\\s*(\\w*))+");
	QStringList list;
	int pos = 0;

	while ((pos = rx.indexIn(statement, pos)) != -1) {
		int j = 1;
		QString c;

		while (!(c = rx.cap(j++)).isEmpty()) {
			list << c;
		}

		newStatement = newStatement.replace(rx.cap(0), QString("CONCAT(%1)").arg(list.join(",")));

		list.clear();

		pos += rx.matchedLength();
	}

	//qDebug() <<  set << "\n\t" << QString("CONCAT(%1)").arg(list.join(","));
	//qDebug() << statement << "\n\t" << newStatement;

	return newStatement;
}

QString SQLMySqlDatabase::createViewQuery(const QString& viewName, const QString& selectStatement) const {
	return QString("CREATE VIEW %1 AS %2").arg(viewName).arg(selectStatement);
}

bool SQLMySqlDatabase::transaction() const {
	// unfortunately the QMYSQL driver seems to have a problem with transactions so we forcibly disable autocommit
	QSqlQuery autocommit(database());
	if (autocommit.exec("SET autocommit=0;")) {
		qDebug() << "SQLMySqlDatabase::transaction: set autocommit to 0";
	}
	else {
		qDebug() << "SQLMySqlDatabase::transaction: setting autocommit to 0 failed:" << autocommit.lastError();
	}

	return database().transaction();
}

bool SQLMySqlDatabase::commit() const {
	QSqlQuery autocommit(database());
	if (autocommit.exec("SET autocommit=1;")) {
		qDebug() << "SQLMySqlDatabase::transaction: set autocommit to 0";
	}
	else {
		qDebug() << "SQLMySqlDatabase::transaction: setting autocommit to 0 failed:" << autocommit.lastError();
	}

	return database().commit();
}

void SQLMySqlDatabase::setPragmas() {

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

	//qDebug() << "SQLMySqlDatabase::tableFields: returned" << fields;

	return fields;
}
