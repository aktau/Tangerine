#ifndef SQLITEDATABASE_H_
#define SQLITEDATABASE_H_

#include "SQLDatabase.h"

class SQLiteDatabase : public SQLDatabase {
		Q_OBJECT

	public:
		//virtual void connect(const QString& name);
		//virtual void loadFromXML(const QString& XMLFile);

		//virtual QString fieldSqlType(const QString& field) const;

	public:
		SQLiteDatabase(QObject *parent);
		virtual ~SQLiteDatabase();

		//virtual QSqlDatabase open(const QString& file);
	protected:
		virtual QStringList tables(QSql::TableType type = QSql::Tables) const;
		virtual QString createViewQuery(const QString& viewName, const QString& selectStatement) const;
		virtual void setPragmas();
		virtual QSet<QString> tableFields(const QString& tableName) const;

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLiteDatabase(const SQLiteDatabase&);
		SQLiteDatabase& operator=(const SQLiteDatabase&);

	private:
		static const QString DB_TYPE;

	/*
	private:
		friend class SQLDatabase;
	*/
};

#endif /* SQLITEDATABASE_H_ */
