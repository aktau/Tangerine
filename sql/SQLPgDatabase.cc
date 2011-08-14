#include "SQLPgDatabase.h"

const QString SQLPgDatabase::DB_TYPE = "QPSQL";
const QSet<SQLDatabase::SpecialCapabilities> SQLPgDatabase::SPECIAL_POSTGRESQL = QSet<SQLDatabase::SpecialCapabilities>() << NEED_TYPECAST_NUMERIC_POSTGRESQL;

SQLPgDatabase::SQLPgDatabase(QObject *parent) : SQLDatabase(parent, DB_TYPE) {
	// TODO Auto-generated constructor stub

}

SQLPgDatabase::~SQLPgDatabase() {
	// drop materialized views (this should be disabled at some point, right now I want to debug)

	QSqlQuery query(database());

	foreach (const QString& meta, mViewMatchFields) {
		if (!query.exec(QString("SELECT drop_matview('%1')").arg(meta))) {
			qDebug() << "SQLPgDatabase::~SQLPgDatabase: Couldn't remove view" << meta << query.lastError();
		}
	}
}

QSet<SQLDatabase::SpecialCapabilities> SQLPgDatabase::supportedCapabilities() const { return SPECIAL_POSTGRESQL; }
bool SQLPgDatabase::supports(SpecialCapabilities capability) const { return SPECIAL_POSTGRESQL.contains(capability); }

QString SQLPgDatabase::createViewQuery(const QString& viewName, const QString& selectStatement) const {
	return QString("CREATE OR REPLACE VIEW %1 AS (%2);").arg(viewName).arg(selectStatement);
}

void SQLPgDatabase::setPragmas() {
	// load procedures

	/*
	CREATE OR REPLACE FUNCTION create_matview(name, name)
	RETURNS void AS
	$BODY$
	    DECLARE
	        matview ALIAS FOR $1;
	        view_name ALIAS FOR $2;
	        entry matviews%ROWTYPE;
	    BEGIN
	        SELECT * INTO entry FROM matviews WHERE mv_name = quote_ident(matview);
	        IF FOUND THEN
	            RAISE EXCEPTION 'Materialized view ''%'' already exists.', quote_ident(matview);
	        END IF;

	        EXECUTE 'REVOKE ALL ON ' || quote_ident(view_name) || ' FROM PUBLIC';
	        EXECUTE 'GRANT SELECT ON ' || quote_ident(view_name) || ' TO PUBLIC';
	        EXECUTE 'CREATE TABLE ' || quote_ident(matview) || ' AS SELECT * FROM ' || quote_ident(view_name);
	        EXECUTE 'REVOKE ALL ON ' || quote_ident(matview) || ' FROM PUBLIC';
	        EXECUTE 'GRANT SELECT ON ' || quote_ident(matview) || ' TO PUBLIC';

	        INSERT INTO matviews (mv_name, v_name, last_refresh)
	        VALUES (quote_ident(matview), quote_ident(view_name), CURRENT_TIMESTAMP);
	        RETURN;
	    END
	$BODY$
	LANGUAGE 'plpgsql' VOLATILE SECURITY DEFINER
	COST 100;
	*/
}

bool SQLPgDatabase::materializeMetaAttributes() {
	// do nothing
	QSqlQuery query(database());

	foreach (const QString& meta, mViewMatchFields) {
		if (!query.exec(QString("ALTER VIEW %1 RENAME TO %1_unmv").arg(meta))) {
			qDebug() << "SQLPgDatabase::materializeMetaAttributes: Couldn't rename view" << meta << query.lastError();
		}
		if (!query.exec(QString("SELECT create_matview('%1', '%1_unmv')").arg(meta))) {
			qDebug() << "SQLPgDatabase::materializeMetaAttributes: Couldn't materialize view" << meta << query.lastError();
		}

		// create indices

		QString index = QString("CREATE UNIQUE INDEX %1_compound_reverse ON %1 USING btree (%1, match_id)").arg(meta);
		if (!query.exec(index)) qDebug() << "Couldn create index " << index << "\nerror:" << query.lastError();

		index = QString("CREATE UNIQUE INDEX %1_compound ON %1 USING btree (match_id, %1)").arg(meta);
		if (!query.exec(index)) qDebug() << "Couldn create index " << index << "\nerror:" << query.lastError();

		index = QString("CREATE INDEX %1_index ON %1 USING btree (%1)").arg(meta);
		if (!query.exec(index)) qDebug() << "Couldn create index " << index << "\nerror:" << query.lastError();

		index = QString("CREATE UNIQUE INDEX %1_fakeprimary ON %1 USING btree (match_id)").arg(meta);
		if (!query.exec(index)) qDebug() << "Couldn create index " << index << "\nerror:" << query.lastError();
	}

	return true;
}

QSet<QString> SQLPgDatabase::tableFields(const QString& tableName) const {
	QSet<QString> fields;

	QSqlQuery query(database());

	if (query.exec(QString("SELECT column_name FROM information_schema.columns WHERE table_name = '%1';").arg(tableName))) {
		while (query.next()) {
			fields << query.value(0).toString();
		}
	}
	else {
		qDebug() << "SQLPgDatabase::: error fetching table info:" << query.lastError();
	}

	//qDebug() << "SQLMySqlDatabase::tableFields: returned" << fields;

	return fields;
}

QString SQLPgDatabase::schemaName() const {
	return "public";
}

void SQLPgDatabase::createHistory(const QString& table) {
	QSqlQuery query(database());

	transaction();

	if (query.exec(QString("CREATE TABLE %1_history AS SELECT * FROM %1 WHERE 1=2").arg(table))) {
		qDebug() << "SQLPgDatabase::createHistory: succesfully created history for" << table;
	}
	else {
		qDebug() << "SQLPgDatabase::createHistory: couldn't create history table for" << table << "->" << query.lastError() << "\n\tExecuted:" << query.lastQuery();
		database().rollback();
	}

	if (
		query.exec(QString("ALTER TABLE %1_history ADD COLUMN user_id INT").arg(table)) &&
		query.exec(QString("ALTER TABLE %1_history ADD COLUMN timestamp INT").arg(table))
	) {
		qDebug() << "SQLPgDatabase::createHistory: succesfully added history column for" << table;
	}
	else {
		qDebug() << "SQLPgDatabase::createHistory: couldn't add columns for" << table << "->" << query.lastError() << "\n\tExecuted:" << query.lastQuery();
		database().rollback();
	}

	commit();
}
