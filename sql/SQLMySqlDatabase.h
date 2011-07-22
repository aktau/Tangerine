#ifndef SQLMYSQLDATABASE_H_
#define SQLMYSQLDATABASE_H_

#include "SQLDatabase.h"

class SQLMySqlDatabase : public SQLDatabase {
	Q_OBJECT

	public:
		SQLMySqlDatabase(QObject *parent);
		virtual ~SQLMySqlDatabase();

	protected:
		virtual void setPragmas() { }
		virtual QSet<QString> tableFields(const QString& /*tableName*/) const { return QSet<QString>(); }

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLMySqlDatabase(const SQLMySqlDatabase&);
		SQLMySqlDatabase& operator=(const SQLMySqlDatabase&);

	private:
		static const QString DB_TYPE;
};

#endif /* SQLMYSQLDATABASE_H_ */
