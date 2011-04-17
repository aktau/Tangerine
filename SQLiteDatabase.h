#ifndef SQLITEDATABASE_H_
#define SQLITEDATABASE_H_

#include "SQLDatabase.h"

class SQLiteDatabase : public SQLDatabase {
	public:
		SQLiteDatabase();
		virtual ~SQLiteDatabase();

		virtual void connect(const QString& name);
		virtual void loadFromXML(const QString& XMLFile);

	protected:
		virtual QSqlDatabase open(const QString& file);
		virtual void setPragmas();
};

#endif /* SQLITEDATABASE_H_ */
