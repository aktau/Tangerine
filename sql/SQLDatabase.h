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

#include "SQLRawTheraRecords.h"

class SQLDatabase;

struct SQLQueryParameters {
	SQLQueryParameters(const QStringList& attributesToPreload = QStringList(), const QString& sortAttribute = QString(), Qt::SortOrder sortOrder = Qt::AscendingOrder, const SQLFilter& _filter = SQLFilter())
		: preloadFields(attributesToPreload), sortField(sortAttribute), order(sortOrder), filter(_filter), offset(-1), limit(-1), isPaginated(false), forceLateRowLookup(false), forceEarlyRowLookup(false), forceLateRowLookupPass(false) {

	}

	// enables temporarily overriding the dabatase option UseLateRowLookup
	// true = late lookup
	// false = early lookup
	void overrideLateLookup(bool late) {
		forceLateRowLookup = late;
		forceEarlyRowLookup = !late;
	}

	// this can be very slow, but is necessary if you don't have a reference value for the current window
	void moveToAbsoluteWindow(int offsetFromZero, int windowSize) {
		isPaginated = false;

		offset = offsetFromZero;
		limit = windowSize;
	}

	// this will make the query relative, which basically means that it will use the pivot value
	// to get the results faster
	//
	// if goForward is true, the relative window will be taken starting from the current pivot going forward
	// else, it's taken going backwards
	void moveToRelativeWindow(const thera::SQLFragmentConf& pivot, bool includePivot, bool goForward, int offsetFromCurrent, int windowSize) {
		isPaginated = true;

		offset = offsetFromCurrent;
		limit = windowSize;

		forward = goForward;

		extremeMatchId = pivot.index();
		extremeSortValue = (!sortField.isEmpty()) ? pivot.getDouble(sortField, 0.0) : 0.0;

		inclusive = includePivot;
	}

private:
	friend class SQLDatabase;

	QStringList preloadFields;
	QStringList preloadMetaFields;
	QString sortField;
	Qt::SortOrder order;
	SQLFilter filter;
	int offset;
	int limit;

	// pagination-only data
	bool isPaginated;

	int extremeMatchId;
	double extremeSortValue;
	bool forward;
	bool inclusive;

	// optimization stuff
	bool forceLateRowLookup;
	bool forceEarlyRowLookup;

	// really, what? optimizations inside of optimizations?
	bool forceLateRowLookupPass;
};

class SQLDatabase : public QObject {
		Q_OBJECT

	public:
		enum Option {
			NoOptions = 0x0000,
			UseViewEncapsulation = 0x0001, // Performs view encapsulation when meta-attributes are requested but not present as a seperate dependency
			UseLateRowLookup = 0x0002,  // An optimization that works especially well with MySQL, can't usually be combined with UseViewEncapsulation, forces the database to only look up rows after collecting id's
			//OptionC = 0x2,  // 0x000010
			//OptionD = 0x4,  // 0x000100
			//OptionE = 0x8,  // 0x001000
			//OptionE = 0x10 // 0x010000
			// ... some more options with value which is a power of two
		 };
		 Q_DECLARE_FLAGS(Options, Option)

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

		// this method will multiply the amount of input data by a factor, so that
		// benchmarks on huge datasets can be ran (it can also perturb the data by a random factor
		// when inserting so that not too many duplicate entries are made). You can use
		// this in case you only possess small datasets
		virtual void stressTestFromXML(const QString& XMLFile, int factor = 10, bool perturb = true);

		virtual bool addMatchField(const QString& name, double defaultValue);
		virtual bool addMatchField(const QString& name, const QString& defaultValue);
		virtual bool addMatchField(const QString& name, int defaultValue);
		virtual bool addMetaMatchField(const QString& name, const QString& sql); // a metafield is a field computed from other fields, it is usually implemented through an SQL view
		virtual bool removeMatchField(const QString& name);

		// in the filters QMap, the key is the field dependencies and the value is the SQL clause that will be put into a WHERE, they will be concatenated with AND
		// one could perfectly also include AND's and OR's inside of the value component
		// example: Key = "error" -> Value = "error < 0.25 OR error > 0.50"
		// other example: Key = "source_name, target_name" -> Value = "(source_name || target_name) LIKE %WDC_0043%"
		thera::SQLFragmentConf getMatch(int id);
		QList<thera::SQLFragmentConf> getMatches(SQLQueryParameters& parameters);
		QList<thera::SQLFragmentConf> getMatches(const QString& sortField = QString(), Qt::SortOrder order = Qt::AscendingOrder, const SQLFilter& filter = SQLFilter(), int offset = -1, int limit = -1);
		QList<thera::SQLFragmentConf> getPreloadedMatches(const QStringList& preloadFields, const QString& sortField = QString(), Qt::SortOrder order = Qt::AscendingOrder, const SQLFilter& filter = SQLFilter(), int offset = -1, int limit = -1);
		QList<thera::SQLFragmentConf> getFastPaginatedPreloadedMatches(const QStringList& preloadFields, const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int limit, int extremeMatchId, double extremeSortValue, bool forward, bool inclusive, int offset);
		int getNumberOfMatches(const SQLFilter& filter = SQLFilter()) const;

		bool historyAvailable() const;
		QList<HistoryRecord> getHistory(const QString& field, const QString& sortField = QString(), Qt::SortOrder order = Qt::AscendingOrder, const SQLFilter& filter = SQLFilter(), int offset = -1, int limit = -1);

		// you can see this as a simplified version of getHistory(), it will return all the current values for the attribute of each match
		QList<AttributeRecord> getAttribute(const QString& field);

		// the following method will try to convert any standard function that is not available
		// in the instantiated DB type into a specialized function, an example:
		// ANSI string concatenation: 'foo' || 'bar' = 'foobar'
		// MySQL doesn't understand this, but does understand: CONCAT('foo','bar') = 'foobar'
		// So makeCompatible will convert double pipes if the database is MySQL
		virtual QString makeCompatible(const QString& statement) const;
		virtual QString escapeCharacter() const; // will return an empty string if you have to define an escape character yourself (with ESCAPE '\' for example

		bool matchHasField(const QString& field) const;
		const QSet<QString>& matchFields() const;

		bool matchHasRealField(const QString& field) const;
		const QSet<QString>& realMatchFields() const;

		int matchCount() const;

		virtual bool transaction() const;
		virtual bool commit() const;

		// if the 'id' parameters is -1, a new id is created
		//		WARNING: if an id is specified and a configuration with the same id already existed, the results are UNDEFINED
		// returns the fragment conf of the inserted match
		// the fragment conf will be invalid if the query failed (index == -1)
		virtual thera::SQLFragmentConf addMatch(const QString& sourceName, const QString& targetName, const thera::XF& xf, int id = -1);
		// virtual thera::SQLFragmentConf addMatch(const thera::IfragmentConf& conf);

		virtual void setOptions(SQLDatabase::Options options);
		virtual void setOption(SQLDatabase::Option option, bool enable = true);
		virtual SQLDatabase::Options options() const;

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
		// for internal usage for now
		typedef enum {
			FORCE_INDEX_MYSQL // database can force specific index usage with MySQL syntax
		} SpecialCapabilities;

	protected:
		virtual QString extUuid();

		QList<thera::SQLFragmentConf> getPreloadedMatchesFast(const QStringList& preloadFields, const QString& sortField = QString(), Qt::SortOrder order = Qt::AscendingOrder, const SQLFilter& filter = SQLFilter(), int offset = -1, int limit = -1);

		virtual QString synthesizeQuery(SQLQueryParameters& parameters, SQLDatabase::Options options);

		virtual QString synthesizeQuery(const QStringList& requiredFields, const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int offset, int limit) const;
		// the next one has a lot of arguments, they're commented in the method
		virtual QString synthesizeFastPaginatedQuery(const QStringList& requiredFields, const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int limit, int extremeMatchId, double extremeSortValue, bool forward, bool inclusive, int offset) const;
		virtual QList<thera::SQLFragmentConf> fillFragments(const QString& query, const QStringList& cacheFields);

		virtual bool open(const QString& connName, const QString& dbname, bool dbnameOnly, const QString& host = QString(), const QString& user = QString(), const QString& pass = QString(), int port = 0);
		virtual bool reopen();

		virtual bool hasCorrectCapabilities() const;

		virtual QString schemaName() const;
		virtual QStringList tables(QSql::TableType type = QSql::Tables) const;
		virtual QSet<QString> tableFields(const QString& tableName) const = 0;

		virtual QSet<SQLDatabase::SpecialCapabilities> supportedCapabilities() const;
		virtual bool supports(SpecialCapabilities capability) const;

		virtual void setPragmas() = 0;
		virtual void setConnectOptions() const;

		virtual QString createViewQuery(const QString& viewName, const QString& selectStatement) const = 0;

		virtual void createIndex(const QString& table, const QStringList& fields);

		QSqlDatabase database() const;
		void reset();
		void setup(const QString& schemaFile);

		// only for use in getDb
		void setConnectionName(const QString& connectionName);

		// fetches a specific query by key and makes it if it doesn't exist
		QSqlQuery& getOrElse(const QString& key, const QString& queryString);

	protected slots:
		void createHistory();
		virtual void createHistory(const QString& table);

		virtual void resetQueries();
		virtual void makeFieldsSet();

	private:
		void parseXML(const QDomElement &root);
		void parseXMLStressTest(const QDomElement &root, int factor, bool perturb);
		const QDomDocument toXML();

		// doesn't send the matchFieldsChanged() singal, you have to do that yourself if necessary
		template<typename T> bool addMatchField(const QString& name, const QString& sqlType, T defaultValue, bool indexValue = true);

		template<typename T> void matchSetValue(int id, const QString& field, const T& value);
		template<typename T> T matchGetValue(int id, const QString& field, const T& deflt) const;

		template<typename T> void matchAddHistoryRecord(int id, const QString& field, const T& value);

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLDatabase(const SQLDatabase&);
		SQLDatabase& operator=(const SQLDatabase&);

	protected:
		Options mOptions;

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

Q_DECLARE_OPERATORS_FOR_FLAGS(SQLDatabase::Options)

inline const QSet<QString>& SQLDatabase::matchFields() const {
	return mMatchFields;
}

inline bool SQLDatabase::matchHasField(const QString& field) const {
	return mMatchFields.contains(field.toLower());
}

inline const QSet<QString>& SQLDatabase::realMatchFields() const {
	return mNormalMatchFields;
}

inline bool SQLDatabase::matchHasRealField(const QString& field) const {
	return mNormalMatchFields.contains(field.toLower());
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

#endif /* SQLDATABASE_H_ */
