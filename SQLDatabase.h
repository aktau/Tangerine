#ifndef SQLDATABASE_H_
#define SQLDATABASE_H_

#include <QObject>
#include <QtSql>
#include <QDomDocument>
#include <QList>
#include <QSet>
#include <QMap>

#include "SQLFragmentConf.h"
#include "SQLFilter.h"

class SQLDatabase : public QObject {
		Q_OBJECT

	public:
		static SQLDatabase* getDatabase(QObject *parent = QCoreApplication::instance());

	protected:
		SQLDatabase(QObject *parent);
		virtual ~SQLDatabase();

	public:

		bool isOpen() const;

		// we could probably provide a base implementation fo all non-SQLite db's
		virtual void connect(const QString& name) = 0;

		virtual void loadFromXML(const QString& XMLFile);
		virtual void saveToXML(const QString& XMLFile) const;

		virtual bool addMatchField(const QString& name, double defaultValue);
		virtual bool addMatchField(const QString& name, const QString& defaultValue);
		virtual bool addMatchField(const QString& name, int defaultValue);
		virtual bool removeMatchField(const QString& name);
		//virtual QString fieldSqlType(const QString& field) const = 0;

		// in the filters map, the key is the field dependencies and the value is the SQL clause that will be put into a WHERE, they will be concatenated with AND
		// one could perfectly also include AND's and OR's inside of the value component
		// example: Key = "error" -> Value = "error < 0.25 OR error > 0.50"
		// other example: Key = "source_name, target_name" -> Value = "(source_name || target_name) LIKE %WDC_0043%"
		QList<thera::SQLFragmentConf> getMatches(const QString& sortField = QString(), Qt::SortOrder order = Qt::AscendingOrder, const SQLFilter& filter = SQLFilter());

		bool matchHasField(const QString& field) const;
		const QSet<QString>& matchFields() const;

		int matchCount() const;

	signals:
		void databaseOpened();
		void databaseClosed();
		void databaseOpStarted(const QString& operation, int steps);
		void databaseOpStepDone(int step);
		void databaseOpEnded();
		void matchCountChanged();
		void matchFieldsChanged();

	protected:
		virtual bool hasCorrectCapabilities() const;

		virtual QSqlDatabase open(const QString& file) = 0;
		virtual void setPragmas() = 0;
		virtual QSet<QString> tableFields(const QString& tableName) const = 0;

		QSqlDatabase database() const;
		void close();
		void reset();
		void setup(const QString& schemaFile);

	protected slots:
		virtual void resetQueries();
		virtual void makeFieldsSet();

	private:
		void parseXML(const QDomElement &root);
		const QDomDocument toXML() const;

		template<typename T> bool addMatchField(const QString& name, const QString& sqlType, T defaultValue);

		template<typename T> void matchSetValue(int id, const QString& field, const T& value);
		template<typename T> T matchGetValue(int id, const QString& field);

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLDatabase(const SQLDatabase&);
		SQLDatabase& operator=(const SQLDatabase&);

	protected:
		// a map that will store prepared queries, for performance reasons
		typedef QMap<QString, QSqlQuery *> FieldQueryMap;
		FieldQueryMap mFieldQueryMap;

		// a set that stores all the available fields/attributes for matches
		typedef QSet<QString> MatchFieldSet;
		MatchFieldSet mMatchFields;

		static const QString CONN_NAME;
		static const QString SCHEMA_FILE;
		static const QString DB_HOST;

	private:
		static const QString MATCHES_ROOTTAG;
		static const QString MATCHES_DOCTYPE;
		static const QString OLD_MATCHES_VERSION;
		static const QString MATCHES_VERSION;

		static SQLDatabase *mSingleton;

	private:
		friend class thera::SQLFragmentConf;
};

inline bool SQLDatabase::matchHasField(const QString& field) const {
	return mMatchFields.contains(field.toLower());
}

template<typename T> inline void SQLDatabase::matchSetValue(int id, const QString& field, const T& value) {

}

template<typename T> inline T SQLDatabase::matchGetValue(int id, const QString& field) {
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
			qDebug() << "SQLDatabase::matchGetValue: no record was returned";
		}
	}
	else {
		qDebug()
			<< "SQLDatabase::matchGetValue: Query failed:" << query.lastError()
			<< "\nQuery executed: " << query.executedQuery();
			//<< "\nBound values:" <<query.boundValues();
	}

	return QVariant(0).value<T>();
}

inline QSqlDatabase SQLDatabase::database() const {
	return QSqlDatabase::database(CONN_NAME);
}

inline const QSet<QString>& SQLDatabase::matchFields() const {
	return mMatchFields;
}

#endif /* SQLDATABASE_H_ */
