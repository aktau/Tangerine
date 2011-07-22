#include "SQLMySqlDatabase.h"

const QString SQLMySqlDatabase::DB_TYPE = "QMYSQL";

SQLMySqlDatabase::SQLMySqlDatabase(QObject *parent) : SQLDatabase(parent, DB_TYPE) {
	// TODO Auto-generated constructor stub

}

SQLMySqlDatabase::~SQLMySqlDatabase() {
	// TODO Auto-generated destructor stub
}
