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

		// TODO: we could probably deliver a base implementation for general SQL databases and overrie ind SQLite
		virtual void connect(const QString& name) = 0;

		virtual void loadFromXML(const QString& XMLFile);
		void saveToXML(const QString& XMLFile) const;
		QDomDocument toXML() const;

		/* matches */

		int matchCount() const;

	signals:
		void databaseOpened();
		void databaseClosed();
		void databaseOpStarted(const QString& operation, int steps);
		void databaseOpStepDone(int step);
		void databaseOpEnded();

		void matchCountChanged();

	protected:
		virtual bool hasCorrectCapabilities() const;
		virtual QSqlDatabase open(const QString& file) = 0;
		virtual void setPragmas() = 0;

		QSqlDatabase database() const;
		void close();
		void reset();
		void setup(const QString& schemaFile);

	private:
		void parseXML(const QDomElement &root);

	protected:
		static const QString CONN_NAME;
		static const QString SCHEMA_FILE;
		static const QString DB_TYPE;
		static const QString DB_HOST;

	private:
		static const QString MATCHES_ROOTTAG;
		static const QString MATCHES_DOCTYPE;
		static const QString OLD_MATCHES_VERSION;
		static const QString MATCHES_VERSION;
};

#endif /* SQLDATABASE_H_ */
