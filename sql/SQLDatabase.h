#ifndef SQLDATABASE_H_
#define SQLDATABASE_H_

#include <QObject>
#include <QtSql>
#include <QDomDocument>
#include <QList>
#include <QSet>
#include <QMap>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QStringBuilder>

#include "SQLFragmentConf.h"
#include "SQLFilter.h"

#include "MatchHistory.h"

class SQLDatabase : public QObject {
		Q_OBJECT

	public:
		SQLDatabase(QObject *parent, const QString& type, bool trackHistory = true);
		virtual ~SQLDatabase();

	public:
		bool isOpen() const;
		virtual bool detectClosedDb() const; // a special routine that will detect closing of the connection even if the normal isOpen() call wouldn't, is more expensive though

		// reads connection data from a .dbd file describing the database to connect to
		// if the file is .xml or .dbd it will be read as a database connection parameters description
		// file. If the extension is .db the function will directly assume that the it's an SQLite database
		static QSharedPointer<SQLDatabase> getDb(const QString& file, QObject *parent = NULL);
		virtual void saveConnectionInfo(const QString& file) const; // will only write a file if isOpen() returns true, if it's a SQLite database it will make a copy of the database to this location

		virtual QString connectionName() const;

		virtual void loadFromXML(const QString& XMLFile);
		virtual void saveToXML(const QString& XMLFile);

		virtual bool addMatchField(const QString& name, double defaultValue);
		virtual bool addMatchField(const QString& name, const QString& defaultValue);
		virtual bool addMatchField(const QString& name, int defaultValue);
		virtual bool addMetaMatchField(const QString& name, const QString& sql); // a metafield is a field computed from other fields, it is implemented through an SQL view
		virtual bool removeMatchField(const QString& name);

		// in the filters QMap, the key is the field dependencies and the value is the SQL clause that will be put into a WHERE, they will be concatenated with AND
		// one could perfectly also include AND's and OR's inside of the value component
		// example: Key = "error" -> Value = "error < 0.25 OR error > 0.50"
		// other example: Key = "source_name, target_name" -> Value = "(source_name || target_name) LIKE %WDC_0043%"
		thera::SQLFragmentConf getMatch(int id);
		QList<thera::SQLFragmentConf> getMatches(const QString& sortField = QString(), Qt::SortOrder order = Qt::AscendingOrder, const SQLFilter& filter = SQLFilter(), int offset = -1, int limit = -1);
		int getNumberOfMatches(const SQLFilter& filter = SQLFilter()) const;

		bool historyAvailable() const;
		//QList<MatchHistory> getHistory(const QString& field);
		QList<HistoryRecord> getHistory(const QString& field, const QString& sortField = QString(), Qt::SortOrder order = Qt::AscendingOrder, const SQLFilter& filter = SQLFilter(), int offset = -1, int limit = -1);
		//QMap<QString, QList<HistoryRecord> > getHistory();

		// the following method will try to convert any standard function that is not available
		// in the instantiated DB type into a specialized function, an example:
		// ANSI string contatenation: 'foo' || 'bar' = 'foobar'
		// MySQL doesn't understand this, but does understand: CONCAT('foo','bar') = 'foobar'
		// So makeCompatible will convert double pipes if the database is MySQL
		virtual QString makeCompatible(const QString& statement) const;

		bool matchHasField(const QString& field) const;
		const QSet<QString>& matchFields() const;

		int matchCount() const;

		virtual bool transaction() const;
		virtual bool commit() const;

		// if the 'id' parameters is -1, a new id is created
		//		WARNING: if an id is specified and a configuration with the same id already existed, the results are UNDEFINED
		// returns the fragment conf of the inserted match
		// the fragment conf will be invalid if the query failed (index == -1)
		virtual thera::SQLFragmentConf addMatch(const QString& sourceName, const QString& targetName, const thera::XF& xf, int id = -1);

	signals:
		void databaseOpened();
		void databaseClosed();
		void databaseOpStarted(const QString& operation, int steps);
		void databaseOpStepDone(int step);
		void databaseOpEnded();
		void matchCountChanged();
		void matchFieldsChanged();

	public slots:
		void close();

	protected:
		virtual bool open(const QString& connName, const QString& dbname, bool dbnameOnly, const QString& host = QString(), const QString& user = QString(), const QString& pass = QString(), int port = 0);
		virtual bool reopen();

		virtual bool hasCorrectCapabilities() const;

		virtual QStringList tables(QSql::TableType type = QSql::Tables) const;
		virtual QSet<QString> tableFields(const QString& tableName) const = 0;

		virtual void setPragmas() = 0;
		virtual void setConnectOptions() const;

		virtual QString createViewQuery(const QString& viewName, const QString& selectStatement) const = 0;

		QSqlDatabase database() const;
		void reset();
		void setup(const QString& schemaFile);

		// history methods
		void createHistory();
		virtual void createHistory(const QString& table);

		// only for use in getDb
		void setConnectionName(const QString& connectionName);

		// fetches a specific query by key and makes it if it doesn't exist
		QSqlQuery& getOrElse(const QString& key, const QString& queryString);

	protected slots:
		virtual void resetQueries();
		virtual void makeFieldsSet();

	private:
		void parseXML(const QDomElement &root);
		const QDomDocument toXML();

		template<typename T> bool addMatchField(const QString& name, const QString& sqlType, T defaultValue);

		template<typename T> void matchSetValue(int id, const QString& field, const T& value);
		template<typename T> T matchGetValue(int id, const QString& field, const T& deflt) const;

		template<typename T> void matchAddHistoryRecord(int id, const QString& field, const T& value);

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLDatabase(const SQLDatabase&);
		SQLDatabase& operator=(const SQLDatabase&);

	protected:
		QString mConnectionName;
		QString mType;

		// a map that will store prepared queries, for performance reasons
		// it's not actually necessary but it speeds things up and will
		// be created on the fly if empty
		typedef QMap<QString, QSqlQuery *> FieldQueryMap;
		mutable FieldQueryMap mFieldQueryMap;

		// a set that stores all the available fields/attributes for matches
		typedef QSet<QString> MatchFieldSet;
		MatchFieldSet mMatchFields;
		MatchFieldSet mNormalMatchFields; // fields that exist as real database tables
		MatchFieldSet mViewMatchFields; // fiels that exists solely as views

		bool mTrackHistory;

	private:
		static const QString SCHEMA_FILE;

		static const QString MATCHES_ROOTTAG;
		static const QString MATCHES_DOCTYPE;
		static const QString OLD_MATCHES_VERSION;
		static const QString MATCHES_VERSION;

		static QHash<QString, QWeakPointer<SQLDatabase> > mActiveConnections;

	private:
		friend class thera::SQLFragmentConf;
};

inline bool SQLDatabase::matchHasField(const QString& field) const {
	return mMatchFields.contains(field.toLower());
}

inline QSqlQuery& SQLDatabase::getOrElse(const QString& key, const QString& queryString) {
	FieldQueryMap::const_iterator i = mFieldQueryMap.constFind(key);

	if (i == mFieldQueryMap.constEnd()) {
		QSqlQuery *query = new QSqlQuery(database());

		query->prepare(queryString);

		i = mFieldQueryMap.insert(key, query);
	}

	return *(i.value());
}

template<typename T> inline void SQLDatabase::matchSetValue(int id, const QString& field, const T& value) {
	QSqlQuery &query = getOrElse(field % "update", QString("UPDATE %1 SET %1 = :value WHERE match_id = :match_id").arg(field));

	query.bindValue(":match_id", id);
	query.bindValue(":value", QVariant(value).toString());

	if (!query.exec()) {
		qDebug()
			<< "SQLDatabase::matchSetValue: Query failed:" << query.lastError()
			<< "\nQuery executed: " << query.executedQuery();
			//<< "\nBound values:" <<query.boundValues();
	}
	else if (mTrackHistory) {
		matchAddHistoryRecord(id, field, value);
	}

	query.finish();
}

template<typename T> inline void SQLDatabase::matchAddHistoryRecord(int id, const QString& field, const T& value) {
	QSqlQuery &query = getOrElse(field % "history", QString("INSERT INTO %1_history (timestamp, user_id, match_id, %1) VALUES (:timestamp, :user_id, :match_id, :value)").arg(field));

	query.bindValue(":timestamp", QDateTime::currentDateTime().toTime_t());
	query.bindValue(":user_id", 0);
	query.bindValue(":match_id", id);
	query.bindValue(":value", QVariant(value).toString());
	//query.bindValue(":confidence", 1.0);

	if (!query.exec())
		qDebug() << "SQLDatabase::matchAddHistoryRecord: could not insert history record:" << query.lastError();
}

template<typename T> inline T SQLDatabase::matchGetValue(int id, const QString& field, const T& deflt) const {
	if (!mFieldQueryMap.contains(field)) {
		// doesn't exist yet, make and insert
		QSqlQuery *q = new QSqlQuery(database());

		q->prepare(QString("SELECT %1 FROM %1 WHERE match_id = :match_id").arg(field));

		mFieldQueryMap.insert(field, q);
	}

	QSqlQuery &query = *mFieldQueryMap.value(field);

	query.bindValue(":match_id", id);

	if (query.exec()) {
		if (query.next()) {
			return query.value(0).value<T>();
		}
		else {
			//qDebug() << "SQLDatabase::matchGetValue: no record was returned";
		}
	}
	else {
		qDebug()
			<< "SQLDatabase::matchGetValue: Query failed:" << query.lastError()
			<< "\nQuery executed: " << query.executedQuery();
			//<< "\nBound values:" <<query.boundValues();
	}

	query.finish();

	return deflt;
}

inline QSqlDatabase SQLDatabase::database() const {
	return QSqlDatabase::database(mConnectionName, false);
}

inline const QSet<QString>& SQLDatabase::matchFields() const {
	return mMatchFields;
}

#endif /* SQLDATABASE_H_ */
