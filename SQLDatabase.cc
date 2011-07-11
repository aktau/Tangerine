#include "SQLDatabase.h"

#include <QFile>
#include <QTextStream>

#include "XF.h"

#include "SQLiteDatabase.h"

using namespace thera;

const QString SQLDatabase::CONN_NAME = "TheraSQL";
const QString SQLDatabase::SCHEMA_FILE = "db/schema.sql";
const QString SQLDatabase::DB_HOST = "localhost";

const QString SQLDatabase::MATCHES_ROOTTAG = "matches";
const QString SQLDatabase::MATCHES_DOCTYPE = "matches-cache";
const QString SQLDatabase::OLD_MATCHES_VERSION = "0.0";
const QString SQLDatabase::MATCHES_VERSION = "1.0";

SQLDatabase * SQLDatabase::mSingleton = NULL;

/**
 * TODO: this is _very_ un-threadsafe
 */
SQLDatabase* SQLDatabase::getDatabase(QObject *parent) {
	if (!mSingleton) {
		mSingleton = new SQLiteDatabase(parent);
	}

	return mSingleton;
}

SQLDatabase::SQLDatabase(QObject *parent) : QObject(parent) {
	QObject::connect(this, SIGNAL(databaseClosed()), this, SLOT(resetQueries()));
	QObject::connect(this, SIGNAL(matchFieldsChanged()), this, SLOT(makeFieldsSet()));
}

SQLDatabase::~SQLDatabase() {
	// copying and assigning are allowed now, we'd have to reference count OR rely on some master
	// object calling close() for us
	// TODO: re-evaluate this decision

	close();
}

/*
SQLDatabase::SQLDatabase(const SQLDatabase& that) {
	// for now we're not copying anything, not even the mFieldQueryMap, it automatically regenerates anyway
	// we're not copying that because we don't need too and we're lazy, it would involve free'ing and allocating things!
}

SQLDatabase& SQLDatabase::operator=(const SQLDatabase& that) {
	if (this != &that) {
		// not doing anything in here either,
	}

	return *this;
}
*/

bool SQLDatabase::isOpen() const {
	return database().isValid() && database().isOpen();
}

/**
 * This will allow us to do with a little bit less error-checking at the individual methodlevel
 * It would actually still be advisable to do checks such as:
 * 		if (!db.transaction()) { ... }
 * because there are other things that can go wrong, but we'll leave it for code clarity, for now
 */
bool SQLDatabase::hasCorrectCapabilities() const {
	const QSqlDriver *driver = database().driver();

	if (!driver->hasFeature(QSqlDriver::LastInsertId)) qDebug() << "database doesn't support LastInsertId";
	if (!driver->hasFeature(QSqlDriver::Transactions)) qDebug() << "database doesn't support Transactions";
	if (!(driver->hasFeature(QSqlDriver::NamedPlaceholders) || driver->hasFeature(QSqlDriver::PositionalPlaceholders))) qDebug() << "database doesn't support NamedPlaceholders or PositionalPlaceholders";
	if (!driver->hasFeature(QSqlDriver::PreparedQueries)) qDebug() << "database doesn't support PreparedQueries";

	return (
		driver->hasFeature(QSqlDriver::LastInsertId) &&
		driver->hasFeature(QSqlDriver::Transactions) &&
		(driver->hasFeature(QSqlDriver::NamedPlaceholders) || driver->hasFeature(QSqlDriver::PositionalPlaceholders)) &&
		driver->hasFeature(QSqlDriver::PreparedQueries)
	);
}

void SQLDatabase::loadFromXML(const QString& XMLFile) {
	if (XMLFile == "" || !isOpen()) {
		qDebug("SQLDatabase::loadFromXML: filename was empty or database is not open, aborting...");

		return;
	}

	QFile file(XMLFile);

	// open the file in read-only mode
	if (file.open(QIODevice::ReadOnly)) {
		QDomDocument doc;

		bool succes = doc.setContent(&file);

		file.close();

		if (succes) {
			QDomElement root(doc.documentElement());

			parseXML(root);
		}
		else {
			qDebug() << "Reading XML file" << XMLFile << "failed";
		}
	}
	else {
		qDebug() << "Could not open"  << XMLFile;
	}
}

void SQLDatabase::saveToXML(const QString& XMLFile) {
	if (XMLFile == "") {
		qDebug("SQLDatabase::saveToXML: filename was empty, aborting...");

		return;
	}

	QFile file(XMLFile);

	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QTextStream out(&file);
		QDomDocument doc(toXML());

		doc.save(out, 1);
		file.close();
	}
	else {
		qDebug() << "SQLDatabase::saveToXML: Could not open"  << XMLFile;
	}
}

bool SQLDatabase::addMatchField(const QString& name, double defaultValue) {
	return addMatchField(name, "REAL", defaultValue);
}

bool SQLDatabase::addMatchField(const QString& name, const QString& defaultValue) {
	return addMatchField(name, "TEXT", defaultValue);
}

bool SQLDatabase::addMatchField(const QString& name, int defaultValue) {
	return addMatchField(name, "INTEGER", defaultValue);
}

template<typename T> bool SQLDatabase::addMatchField(const QString& name, const QString& sqlType, T defaultValue) {
	if (matchHasField(name)) {
		qDebug() << "SQLDatabase::addMatchField: field" << name << "already exists";

		return false;
	}

	if (!isOpen()) {
		qDebug() << "SQLDatabase::addMatchField: database wasn't open";

		return false;
	}

	bool success = false;

	QSqlDatabase db = database();
	QSqlQuery query(db);

	db.transaction();

	success = query.exec(QString("CREATE TABLE %1 (match_id INTEGER PRIMARY KEY, %1 %2, confidence REAL)").arg(name).arg(sqlType));
	if (success) {
		// insert the default value everywhere
		query.prepare(QString(
			"INSERT INTO %1 (match_id, %1, confidence) "
			"VALUES (:match_id, :value, :confidence)"
		).arg(name));

		QSqlQuery idQuery(db);
		query.setForwardOnly(true);
		if (idQuery.exec("SELECT match_id FROM matches")) {
			while (idQuery.next()) {
				query.bindValue(":match_id", idQuery.value(0).toInt());
				query.bindValue(":value", defaultValue);
				query.bindValue(":confidence", 1.0);

				query.exec();
			}
		}

		emit matchFieldsChanged();
	}
	else {
		qDebug() << "SQLDatabase::addMatchField couldn't create table:" << query.lastError()
			<< "\nQuery executed:" << query.lastQuery();
	}
	db.commit();

	return success;
}

/**
 * @param name
 * 		The name of the new attribute
 * @param sql
 * 		SQL code to create the view which will create a VIEW that can serve as a regular attribute
 */
bool SQLDatabase::addMetaMatchField(const QString& name, const QString& sql) {
	if (matchHasField(name)) {
		qDebug() << "SQLDatabase::addMetaMatchField: field" << name << "already exists";

		return false;
	}

	if (!isOpen()) {
		qDebug() << "SQLDatabase::addMatchField: database wasn't open";

		return false;
	}

	bool success = false;

	QSqlDatabase db = database();
	QSqlQuery query(db);

	db.transaction();

	success = query.exec(QString("CREATE VIEW IF NOT EXISTS %1 AS %2").arg(name).arg(sql));
	if (success) {
		qDebug() << "SQLDatabase::addMetaMatchField: Create view appears to have been succesful, query:" << query.lastQuery();
		emit matchFieldsChanged();
	}
	else {
		qDebug() << "SQLDatabase::addMetaMatchField: couldn't create VIEW table:" << query.lastError()
			<< "\nQuery executed:" << query.lastQuery();
	}
	db.commit();

	return success;
}

bool SQLDatabase::removeMatchField(const QString& name) {
	if (!matchHasField(name)) {
		qDebug() << "SQLDatabase::removeMatchField: field" << name << "doesn't exist";

		return false;
	}

	if (!isOpen()) {
		qDebug() << "SQLDatabase::removeMatchField: database wasn't open";

		return false;
	}

	// this seems to be necessary for the database table to become "unlocked"
	// even though the queries that we are delete'ing here have in fact had
	// finish() called on them. Anyway, it's not really a big deal.
	//
	// relevant SQL(ite) error if this line is skipped: QSqlError(6, "Unable to fetch row", "database table is locked")
	resetQueries();

	QSqlDatabase db(database());
	QSqlQuery query(db);
	QString queryString;

	if (mNormalMatchFields.contains(name)) queryString = QString("DROP TABLE %1").arg(name);
	else if (mViewMatchFields.contains(name)) queryString = QString("DROP VIEW %1").arg(name);
	else qDebug() << "SQLDatabase::removeMatchField: this should never have happened!";

	db.transaction();
	if (!query.exec(queryString)) {
		qDebug() << "SQLDatabase::removeMatchField couldn't drop table:" << query.lastError()
				<< "\nQuery executed:" << query.lastQuery();
	}
	else {
		emit matchFieldsChanged();
	}
	db.commit();

	return true;
}

int SQLDatabase::getNumberOfMatches(const SQLFilter& filter) const {
	QString queryString = "SELECT Count(matches.match_id) FROM matches";

	QSet<QString> dependencies = filter.dependencies().toSet();

	//join in dependencies
	foreach (const QString& field, dependencies) {
		queryString += QString(" INNER JOIN %1 ON matches.match_id = %1.match_id").arg(field);
	}

	// add filter clauses
	if (!filter.isEmpty()) {
		queryString += " WHERE (" + filter.clauses().join(") AND (") + ")";
	}

	QSqlQuery query(database());
	if (query.exec(queryString) && query.first()) {
		return query.value(0).toInt();
	}
	else {
		qDebug() << "SQLDatabase::getNumberOfMatches: problem with query:" << query.lastError();

		return 0;
	}
}

QList<thera::SQLFragmentConf> SQLDatabase::getMatches(const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int offset, int limit) {
	QList<SQLFragmentConf> list;

	QString queryString = "SELECT matches.match_id, source_name, target_name, transformation FROM matches";

	// join in dependencies
	// << "source_name" << "target_name" << "transformation";
	QSet<QString> dependencies = filter.dependencies().toSet();

	if (!sortField.isEmpty()) {
		if (matchHasField(sortField)) dependencies << sortField;
		else qDebug() << "SQLDatabase::getMatches: attempted to sort on field" << sortField << "which doesn't exist";
	}

	//join in dependencies
	foreach (const QString& field, dependencies) {
		queryString += QString(" INNER JOIN %1 ON matches.match_id = %1.match_id").arg(field);
	}

	// add filter clauses
	if (!filter.isEmpty()) {
		queryString += " WHERE (" + filter.clauses().join(") AND (") + ")";
	}

	if (!sortField.isEmpty()) {
		queryString += QString(" ORDER BY %1.%1 %2").arg(sortField).arg(order == Qt::AscendingOrder ? "ASC" : "DESC");
	}

	if (offset != -1 && limit != -1) {
		queryString += QString(" LIMIT %1, %2").arg(offset).arg(limit);
	}

	qDebug() << "SQLDatabase::getMatches: QUERY =" << queryString;

	QSqlQuery query(database());
	query.setForwardOnly(true);
	if (query.exec(queryString)) {
		while (query.next()) {
			SQLFragmentConf fc(this, query.value(0).toInt());

			//qDebug() << query.value(1).toString() << "->" << Database::entryIndex(query.value(1).toString()) << "||" << query.value(2).toString() << "->" << Database::entryIndex(query.value(2).toString());

			fc.mFragments[IFragmentConf::SOURCE] = Database::entryIndex(query.value(1).toString());
			fc.mFragments[IFragmentConf::TARGET] = Database::entryIndex(query.value(2).toString());
			fc.mRelev = 1.0f; // placeholder; we should compute relev based on err here

			XF xf;
			QTextStream ts(query.value(3).toString().toAscii());
			ts >> xf;

			fc.mXF = xf;

			list.append(fc);
		}
	}
	else {
		qDebug() << "SQLDatabase::getMatches query failed:" << query.lastError()
				<< "\nQuery executed:" << query.lastQuery();
	}

	return list;
}

const QDomDocument SQLDatabase::toXML() {
	if (!isOpen()) {
		qDebug() << "Database wasn't open, couldn't convert to XML";

		return QDomDocument();
	}

	QDomDocument doc(MATCHES_DOCTYPE);
	QDomElement matches = doc.createElement(MATCHES_ROOTTAG);
	matches.setAttribute("version", MATCHES_VERSION);

	const QList<thera::SQLFragmentConf> configurations = getMatches();
	const QStringList fields = matchFields().toList();

	foreach (const thera::SQLFragmentConf& conf, configurations) {
		QDomElement match(doc.createElement("match"));

		match.setAttribute("src", conf.getSourceId());
		match.setAttribute("tgt", conf.getTargetId());
		match.setAttribute("id", QString::number(conf.getID()));

		// all other attributes (the ones stored in the database
		foreach (const QString& field, fields) {
			match.setAttribute(field, conf.getString(field, QString()));
		}

		QString xf;
		for (int col = 0; col < 4; ++col) {
			for (int row = 0; row < 4; ++row) {
				xf += QString("%1 ").arg(conf.mXF[4 * row + col], 0, 'e', 20);
			}
		}

		match.setAttribute("xf", xf);

		matches.appendChild(match);
	}

	doc.appendChild(matches);

	return doc;
}

int SQLDatabase::matchCount() const {
	if (!isOpen()) {
		return 0;
	}

	QSqlQuery query(database());

	if (query.exec("SELECT Count(*) FROM matches") && query.first()) {
		return query.value(0).toInt();
	}
	else {
		qDebug() << "SQLDatabase::matchCount: problem with query:" << query.lastError();

		return 0;
	}
}

/**
 * TODO: batch queries (make VariantList's)
 * TODO: more sanity-checking for corrupt matches.xml
 * TODO: decide if we use the preset matches.xml id, or a new one
 */
void SQLDatabase::parseXML(const QDomElement &root) {
	register int i = 1;

	qDebug() << "STARTING TO PARSE XML";

	QSqlDatabase db(database());

	db.transaction();

	// prepare queries
	QSqlQuery matchesQuery(db);
	matchesQuery.prepare(
		"INSERT INTO matches (match_id, source_id, source_name, target_id, target_name, transformation) "
		"VALUES (:match_id, :source_id, :source_name, :target_id, :target_name, :transformation)"
	);

	QSqlQuery conflictsQuery(db);
	conflictsQuery.prepare(
		"INSERT INTO conflicts (match_id, other_match_id) "
		"VALUES (:match_id, :other_match_id)"
	);

	QSqlQuery statusQuery(db);
	statusQuery.prepare(
		"INSERT INTO status (match_id, status) "
		"VALUES (:match_id, :status)"
	);

	QSqlQuery errorQuery(db);
	errorQuery.prepare(
		"INSERT INTO error (match_id, error) "
		"VALUES (:match_id, :error)"
	);

	QSqlQuery overlapQuery(db);
	overlapQuery.prepare(
		"INSERT INTO overlap (match_id, overlap) "
		"VALUES (:match_id, :overlap)"
	);

	QSqlQuery volumeQuery(db);
	volumeQuery.prepare(
		"INSERT INTO volume (match_id, volume) "
		"VALUES (:match_id, :volume)"
	);

	QSqlQuery old_volumeQuery(db);
	old_volumeQuery.prepare(
		"INSERT INTO old_volume (match_id, old_volume) "
		"VALUES (:match_id, :old_volume)"
	);

	emit databaseOpStarted(tr("Converting XML file to database"), root.childNodes().length());

	for (QDomElement match = root.firstChildElement("match"); !match.isNull(); match = match.nextSiblingElement()) {
		// TODO: how can we check that this isn't already in the table? for now assume clean table

		int matchId = match.attribute("id").toInt();
		//int matchId = query.lastInsertId().toInt();

		//QString debug = QString("item %1: source = %2, target = %3 || (id = %4)").arg(i).arg(match.attribute("src")).arg(match.attribute("tgt")).arg(matchId);
		//qDebug() << debug;

		// update matches table

		//XF transformation;
		QString rawTransformation(match.attribute("xf", "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1").toAscii()); // should be QTextStream if we want the real XF
		//rawTransformation >> transformation;

		matchesQuery.bindValue(":match_id", matchId);
		matchesQuery.bindValue(":source_id", 0); // TODO: not use dummy value
		matchesQuery.bindValue(":source_name", match.attribute("src"));
		matchesQuery.bindValue(":target_id", 0); // TODO: not use dummy value
		matchesQuery.bindValue(":target_name", match.attribute("tgt"));
		matchesQuery.bindValue(":transformation", rawTransformation);
		matchesQuery.exec();

		// update attribute tables
		statusQuery.bindValue(":match_id", matchId);
		statusQuery.bindValue(":status", match.attribute("status", "0").toInt());
		statusQuery.exec();

		errorQuery.bindValue(":match_id", matchId);
		errorQuery.bindValue(":error", match.attribute("error", "NaN").toDouble());
		errorQuery.exec();

		overlapQuery.bindValue(":match_id", matchId);
		overlapQuery.bindValue(":overlap", match.attribute("overlap", "0.0").toDouble());
		overlapQuery.exec();

		volumeQuery.bindValue(":match_id", matchId);
		volumeQuery.bindValue(":volume", match.attribute("volume", "0.0").toDouble());
		volumeQuery.exec();

		old_volumeQuery.bindValue(":match_id", matchId);
		old_volumeQuery.bindValue(":old_volume", match.attribute("old_volume", "0.0").toDouble());
		old_volumeQuery.exec();

		// update conflicts table
		/*
		QString conflictString = match.attribute("conflict", "");
		QStringList conflicts = conflictString.split(" ");

		foreach (const QString &c, conflicts) {
			conflictsQuery.bindValue(":match_id", matchId);
			conflictsQuery.bindValue(":other_match_id", c.toInt());
			conflictsQuery.exec();
		}
		*/

		emit databaseOpStepDone(i);

		++i;
	}

	db.commit();

	qDebug() << "DONE TO PARSE XML";

	emit databaseOpEnded();
	emit matchCountChanged();
}

void SQLDatabase::reset() {
	// TODO: disconnect and unlink db file + call setup
}

void SQLDatabase::setup(const QString& schemaFile) {
	// get database
	QSqlDatabase db = database();

	// read schema file
	QFile file(schemaFile);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Schema file '" << schemaFile << "' could not be opened, aborting";

		return;
	}

	QByteArray data(file.readAll());
	file.close();

	QString schemaQuery = QString(data);
	QStringList queries = schemaQuery.split(";");
	QSqlQuery query(db);

	db.transaction();
	foreach (const QString &q, queries) {
		query.exec(q);
		qDebug() << "Executed query:" << q;
	}
	db.commit();

	emit matchFieldsChanged();
}

void SQLDatabase::close() {
	if (isOpen()) {
		qDebug() << "SQLDatabase::close: Closing database";

		database().close();
		QSqlDatabase::removeDatabase(CONN_NAME);

		emit databaseClosed();
	}
	else {
		qDebug() << "SQLDatabase::close: Couldn't close current database because it wasn't open to begin with";
	}
}

void SQLDatabase::resetQueries() {
	qDeleteAll(mFieldQueryMap);

	mFieldQueryMap.clear();

	qDebug() << "SQLDatabase::resetQueries: reset queries";
}

void SQLDatabase::makeFieldsSet() {
	if (!isOpen()) return;

	// clear just in case
	mMatchFields.clear();

	QSqlDatabase db = database();
	QSqlQuery query(db);

	foreach (const QString& table, db.tables()) {
		// check if the tables name is not the 'matches' table itself
		if (table != "matches") {
			// check if the table contains a match_id attribute
			if (tableFields(table).contains("match_id")) {
				mNormalMatchFields << table;
				mMatchFields << table;
			}
		}
	}

	// include the view attributes as well but add them to mViewMatchFields as well so we can differentiate them later
	// from the normal ones
	foreach (const QString& table, db.tables(QSql::Views)) {
		// check if the tables name is not the 'matches' table itself
		if (table != "matches") {
			// check if the table contains a match_id attribute
			if (tableFields(table).contains("match_id")) {
				mViewMatchFields << table;
				mMatchFields << table;
			}
		}
	}

	// add the default attributs that are special and always there (their "special" status may dissapear later though)
	//mMatchFields << "source_id" << "source_name" << "target_id" << "target_name" << "transformation";
}
