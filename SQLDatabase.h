#ifndef SQLDATABASE_H_
#define SQLDATABASE_H_

#include <QObject>
#include <QtSql>
#include <QDomDocument>
#include <QList>
#include <QStringList>
#include <QMap>

#include "SQLFragmentConf.h"

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

		// using QList because QVector require a default constructor, which we _cannot_ over with SQLFragmentConf
		// alternative solutions are:
		// 1) use a pointer (but makes ownership dangerous), advantage would be polymorhpy
		// 2) ...
		// 3) Profit, I think
		//QList<thera::SQLFragmentConf> getAllMatches();
		QList<thera::SQLFragmentConf> getMatches(const QString& sortField = QString(), Qt::SortOrder order = Qt::AscendingOrder, const QString& filter = QString());

		QStringList fieldList() const;

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

	protected slots:
		virtual void resetQueries();

	private:
		void parseXML(const QDomElement &root);
		const QDomDocument toXML() const;

		bool matchHasField(const QString& field) const;
		template<typename T> void matchSetValue(int id, const QString& field, const T& value);
		template<typename T> T matchGetValue(int id, const QString& field);

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLDatabase(const SQLDatabase&);
		SQLDatabase& operator=(const SQLDatabase&);

	protected:
		typedef QMap<QString, QSqlQuery *> FieldQueryMap;
		FieldQueryMap mFieldQueryMap;

		static const QString CONN_NAME;
		static const QString SCHEMA_FILE;
		static const QString DB_HOST;

	private:
		static const QString MATCHES_ROOTTAG;
		static const QString MATCHES_DOCTYPE;
		static const QString OLD_MATCHES_VERSION;
		static const QString MATCHES_VERSION;

		static QStringList FIELDS;

		static SQLDatabase *mSingleton;

	private:
		friend class thera::SQLFragmentConf;
};

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

#endif /* SQLDATABASE_H_ */
