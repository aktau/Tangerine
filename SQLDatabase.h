#ifndef SQLDATABASE_H_
#define SQLDATABASE_H_

class SQLDatabase {
	public:
		SQLDatabase(const QString& dbFile);
		virtual ~SQLDatabase();

		void loadFromXML(const QString& XMLFile);

	private:
		void resetDB();
		void setupDB(const QString& file);

		QSqlDatabase *mDb;
};

#endif /* SQLDATABASE_H_ */
