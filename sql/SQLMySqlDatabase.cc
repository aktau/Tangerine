#include "SQLMySqlDatabase.h"

const QString SQLMySqlDatabase::DB_TYPE = "QMYSQL";
const QSet<SQLDatabase::SpecialCapabilities> SQLMySqlDatabase::SPECIAL_MYSQL = QSet<SQLDatabase::SpecialCapabilities>() << FORCE_INDEX_MYSQL;

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

QSet<SQLDatabase::SpecialCapabilities> SQLMySqlDatabase::supportedCapabilities() const { return SPECIAL_MYSQL; }
bool SQLMySqlDatabase::supports(SpecialCapabilities capability) const { return SPECIAL_MYSQL.contains(capability); }

QString SQLMySqlDatabase::createViewQuery(const QString& viewName, const QString& selectStatement) const {
	return QString("CREATE OR REPLACE VIEW `%1` AS (%2);").arg(viewName).arg(selectStatement);
}

QString SQLMySqlDatabase::escapeCharacter() const {
	return QString("\\");
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
		qDebug() << "SQLMySqlDatabase::transaction: set autocommit to 1";
	}
	else {
		qDebug() << "SQLMySqlDatabase::transaction: setting autocommit to 1 failed:" << autocommit.lastError();
	}

	return database().commit();
}

void SQLMySqlDatabase::setPragmas() {
	QSqlQuery q(database());

	QString charset = "utf8";
	QString collation = "utf8_general_ci";

	/*
	if (!q.exec(QString("set character_set_client=%1;").arg(charset))) qDebug() << "Error:" << q.lastQuery() << "\n\t" << q.lastError();
	if (!q.exec(QString("set character_set_connection=%1;").arg(charset))) qDebug() << "Error:" << q.lastQuery() << "\n\t" << q.lastError();
	if (!q.exec(QString("set character_set_results=%1;").arg(charset))) qDebug() << "Error:" << q.lastQuery() << "\n\t" << q.lastError();
	//if (!q.exec("set character_set_system=latin1;")) qDebug() << "Error:" << q.lastQuery() << "\n\t" << q.lastError();

	if (!q.exec(QString("set collation_connection=%1;").arg(collation))) qDebug() << "Error:" << q.lastQuery() << "\n\t" << q.lastError();
	if (!q.exec(QString("set collation_database=%1;").arg(collation))) qDebug() << "Error:" << q.lastQuery() << "\n\t" << q.lastError();
	if (!q.exec(QString("set collation_server=%1;").arg(collation))) qDebug() << "Error:" << q.lastQuery() << "\n\t" << q.lastError();
	*/
}

void SQLMySqlDatabase::setConnectOptions() const {
	//database().exec("set collation_connection=latin1;");

	//database().exec("set collation_connection=utf8_general_ci;");
	//database().setConnectOptions("MYSQL_OPT_RECONNECT=1");

	//qDebug("SQLMySqlDatabase::setConnectOptions: tried to set connection options");
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

// efficient reconnection code from here: http://lists.qt.nokia.com/pipermail/qt-interest/2009-September/012796.html
/*
QVariant v = db.driver()->handle();
if( qstrcmp( v.typeName(), "MYSQL*" ) == 0 )
{
     MYSQL *handle = *static_cast<MYSQL **>( v.data() );
     if( handle != 0 )
     {
	my_bool reconnect = 1;
	mysql_options( handle, MYSQL_OPT_RECONNECT, &reconnect );
     }
}
*/
