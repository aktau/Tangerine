#ifndef SQLITEDATABASE_H_
#define SQLITEDATABASE_H_

#include "SQLDatabase.h"

class SQLiteDatabase : public SQLDatabase {
		Q_OBJECT

	public:
		SQLiteDatabase();
		virtual ~SQLiteDatabase();

		virtual void connect(const QString& name);
		virtual void loadFromXML(const QString& XMLFile);

	protected:
		virtual QSqlDatabase open(const QString& file);
		virtual void setPragmas();

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLiteDatabase(const SQLiteDatabase&);
		SQLiteDatabase& operator=(const SQLiteDatabase&);

	private:
		static const QString DB_TYPE;
};

#endif /* SQLITEDATABASE_H_ */
