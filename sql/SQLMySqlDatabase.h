#ifndef SQLMYSQLDATABASE_H_
#define SQLMYSQLDATABASE_H_

#include "SQLDatabase.h"

class SQLMySqlDatabase : public SQLDatabase {
	Q_OBJECT

	public:
		SQLMySqlDatabase(QObject *parent);
		virtual ~SQLMySqlDatabase();

		virtual QString makeCompatible(const QString& statement) const;
	protected:
		virtual QString createViewQuery(const QString& viewName, const QString& selectStatement) const;
		virtual void setPragmas();
		virtual void setConnectOptions() const;
		virtual bool transaction() const;
		virtual bool commit() const;
		virtual QSet<QString> tableFields(const QString& tableName) const;

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLMySqlDatabase(const SQLMySqlDatabase&);
		SQLMySqlDatabase& operator=(const SQLMySqlDatabase&);

	private:
		static const QString DB_TYPE;
};

#endif /* SQLMYSQLDATABASE_H_ */
