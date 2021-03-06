#ifndef SQLPGDATABASE_H_
#define SQLPGDATABASE_H_

#include "SQLDatabase.h"

class SQLPgDatabase : public SQLDatabase {
	Q_OBJECT

	public:
		SQLPgDatabase(QObject *parent);
		virtual ~SQLPgDatabase();

	protected:
		virtual QSet<SQLDatabase::SpecialCapabilities> supportedCapabilities() const;
		virtual bool supports(SpecialCapabilities capability) const;

		virtual QString createViewQuery(const QString& viewName, const QString& selectStatement) const;
		virtual void setPragmas();
		virtual QSet<QString> tableFields(const QString& tableName) const;
		virtual QString schemaName() const;
		virtual void createHistory(const QString& table);
		virtual bool materializeMetaAttributes();

	private:
		// disabling copy-constructor and copy-assignment for now
		SQLPgDatabase(const SQLPgDatabase&);
		SQLPgDatabase& operator=(const SQLPgDatabase&);

	private:
		static const QString DB_TYPE;
		static const QSet<SQLDatabase::SpecialCapabilities> SPECIAL_POSTGRESQL;
};

#endif /* SQLPGDATABASE_H_ */
