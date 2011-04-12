#ifndef SQLDATABASE_H_
#define SQLDATABASE_H_

#include <QtSql>

class SQLDatabase {
	public:
		SQLDatabase(const QString& dbFilename);
		virtual ~SQLDatabase();

		void loadFromXML(const QString& XMLFile);

	private:
		void resetDB();
		void setupDB(const QString& file);

		static const QString CONN_NAME;
		static const QString DB_TYPE;
		static const QString DB_HOST;
};

#endif /* SQLDATABASE_H_ */
