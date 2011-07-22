#ifndef SQLNULLDATABASE_H_
#define SQLNULLDATABASE_H_

#include "SQLDatabase.h"

class SQLNullDatabase : public SQLDatabase {
	Q_OBJECT

	public:
		SQLNullDatabase(QObject *parent) : SQLDatabase(parent, "NULL") { }
		virtual ~SQLNullDatabase() { }

		//virtual QSqlDatabase open(const QString& file);
	protected:
		virtual void setPragmas() { }
		virtual QSet<QString> tableFields(const QString&) const { return QSet<QString>(); }

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLNullDatabase(const SQLNullDatabase&);
		SQLNullDatabase& operator=(const SQLNullDatabase&);
};

#endif /* SQLNULLDATABASE_H_ */
