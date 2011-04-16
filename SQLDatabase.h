#ifndef SQLDATABASE_H_
#define SQLDATABASE_H_

#include <QObject>
#include <QtSql>
#include <QDomDocument>

class SQLDatabase : public QObject {
		Q_OBJECT

	public:
		SQLDatabase();
		virtual ~SQLDatabase();

		bool isOpen() const;

		void loadFile(const QString& dbFilename);

		void loadFromXML(const QString& XMLFile);
		void saveToXML(const QString& XMLFile) const;
		QDomDocument toXML() const;

	signals:
		void databaseOpened();
		void databaseClosed();
		void databaseOpStarted(const QString& operation, int steps);
		void databaseOpStepDone(int step);
		void databaseOpEnded();

	private:
		void reset();
		void setup(const QString& schemaFile);

		QSqlDatabase database() const;
		QSqlDatabase open(const QString& file);
		void close();

		const QString readSqlFile(const QString& schemaFilename) const;

		void parseXML(const QDomElement &root);

	private:
		static const QString CONN_NAME;
		static const QString SCHEMA_FILE;
		static const QString DB_TYPE;
		static const QString DB_HOST;

		static const QString MATCHES_ROOTTAG;
		static const QString MATCHES_DOCTYPE;
		static const QString OLD_MATCHES_VERSION;
		static const QString MATCHES_VERSION;
};

#endif /* SQLDATABASE_H_ */
