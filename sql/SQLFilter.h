#ifndef SQLFILTER_H_
#define SQLFILTER_H_

#include <QSet>
#include <QHash>
#include <QString>
#include <QStringList>

class SQLDatabase;

class SQLFilter {
	public:
		SQLFilter();
		SQLFilter(SQLDatabase *db);
		virtual ~SQLFilter();

		// all the fields upon which the collection of filters is dependent
		virtual QStringList dependencies() const;

		// the collection of clauses to be put in an SQL WHERE statement, they will usually be combined with the AND operator
		virtual QStringList clauses() const;

		virtual bool isEmpty() const;
		virtual bool hasFilter(const QString& key) const;
		virtual bool hasFilter(const QString& key, const QString& filter) const; // tests if this SQLFilter has this exact combination

		virtual void setDatabase(SQLDatabase *db);
		virtual SQLDatabase *getDatabase() const;

		// for example:
		// 	key: "sourcetargetfilter"
		//	filter: "(source_name || target_name) LIKE '%WDC_0043%'"
		virtual void setFilter(const QString& key, const QString& filter);
		virtual void removeFilter(const QString& key);
		virtual void clear();

		virtual bool operator==(const SQLFilter& other) const;
		virtual bool operator!=(const SQLFilter& other) const;

	protected slots:
		virtual void updateDependencyInfo();

	private:
		QSet<QString> mDependencies;
		QHash<QString, QString> mFilters;

		SQLDatabase *mDb;
};

#endif /* SQLFILTER_H_ */
