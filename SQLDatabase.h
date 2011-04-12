#ifndef SQLDATABASE_H_
#define SQLDATABASE_H_

#include <QtSql>

class SQLDatabase {
	public:
		SQLDatabase(const QString& dbFilename);
		virtual ~SQLDatabase();

		void loadFromXML(const QString& XMLFile);

	private:
		void reset();
		void load(const QString& file);
		void setup(const QString& databaseFile, const QString& schemaFile);

		QSqlDatabase open(const QString& file);
		const QString readSqlFile(const QString& schemaFilename) const;

		static const QString CONN_NAME;
		static const QString SCHEMA_FILE;
		static const QString DB_TYPE;
		static const QString DB_HOST;
};

#endif /* SQLDATABASE_H_ */
