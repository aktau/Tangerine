#define QT_USE_FAST_CONCATENATION
#define QT_USE_FAST_OPERATOR_PLUS

#include "SQLDatabase.h"

#include <QFile>
#include <QTextStream>
#include <QPair>

#include "XF.h"

#include "SQLiteDatabase.h"
#include "SQLMySqlDatabase.h"
#include "SQLNullDatabase.h"

#include "SQLConnectionDescription.h"

using namespace thera;

QHash<QString, QWeakPointer<SQLDatabase> > SQLDatabase::mActiveConnections;

//const QString SQLDatabase::SCHEMA_FILE = "db/schema.sql";
const QString SQLDatabase::SCHEMA_FILE = "config/matches_schema.sql";

const QString SQLDatabase::MATCHES_ROOTTAG = "matches";
const QString SQLDatabase::MATCHES_DOCTYPE = "matches-cache";
const QString SQLDatabase::OLD_MATCHES_VERSION = "0.0";
const QString SQLDatabase::MATCHES_VERSION = "1.0";

QSharedPointer<SQLDatabase> SQLDatabase::getDb(const QString& file, QObject *parent) {
	SQLConnectionDescription dbd(file);
	QSharedPointer<SQLDatabase> db;

	if (dbd.isValid()) {
		// remove connections that have been rendered invalid in the meantime
		QMutableHashIterator<QString, QWeakPointer<SQLDatabase> > i(mActiveConnections);
		while (i.hasNext()) {
			i.next();

			if (i.value().isNull()) {
				qDebug() << "SQLDatabase::getDb: pruned connection" << i.key() << "because it is no longer used by anybody";
				i.remove();
			}
			else {
				// since only opened connections can b e added to the active connections list,
				// finding an unopened database here means it closed by some other means (broken pipe, ...)
				// we should try to reopen and if that fails just remove the connection so that
				// if the user tries to reconnect we don't return this dead connection
				SQLDatabase *odb = i.value().data();
				if (!odb->isOpen() && !odb->reopen()) {
					qDebug() << "SQLDatabase::getDb: pruned connection" << i.key() << "because it is no longer open and cannot be reopened";
					i.remove();
				}
			}
		}

		QString connName = dbd.getConnectionName();

		if (mActiveConnections.contains(connName)) {
			qDebug() << "SQLDatabase::getDb: returned an already active database connection:" << connName;

			db = mActiveConnections.value(connName);
		}
		else {
			switch (dbd.getType()) {
				case SQLConnectionDescription::MYSQL: {
					db = QSharedPointer<SQLDatabase>(new SQLMySqlDatabase(parent));
					db->open(connName, dbd.getDbname(), false, dbd.getHost(), dbd.getUser(), dbd.getPassword(), dbd.getPort());
				} break;

				case SQLConnectionDescription::SQLITE: {
					db = QSharedPointer<SQLDatabase>(new SQLiteDatabase(parent));
					db->open(connName, file, true);
				} break;

				default: {
					qDebug() << "SQLDatabase::getDb: database type unknown, returning unopened dummy database";
					db = QSharedPointer<SQLDatabase>(new SQLNullDatabase(parent));
				} break;
			}

			// note that databases can only be opened through this function (getDb), so a returned database
			// that is closed will stay closed
			assert(!db.isNull());

			if (db->isOpen()) {
				qDebug() << "SQLDatabase::getDb: added this connection to the active connection list:" << connName;

				mActiveConnections.insert(dbd.getConnectionName(), db.toWeakRef());
			}
			else {
				// assure that deleting this object doesn't mess with existing connections, we're going to reset the connection name
				db->setConnectionName(QString());
			}
		}
	}
	else {
		qDebug() << "SQLDatabase::getDb: Database description file" << file << "was invalid, returning invalid database";
		db = QSharedPointer<SQLDatabase>(new SQLNullDatabase(parent));
	}

	return db;
}

void SQLDatabase::saveConnectionInfo(const QString& file) const {
	if (!isOpen()) return;
	// will only write a file if isOpen() returns true, if it's a SQLite database it will make a copy of the database to this location
}

bool SQLDatabase::open(const QString& connName, const QString& dbname, bool dbnameOnly, const QString& host, const QString& user, const QString& pass, int port) {
	if (mConnectionName != connName && QSqlDatabase::database(connName, false).isOpen()) {
		qDebug() << "SQLDatabase::open: Another database with connection name" << connName << "was already opened, close that one first";

		return false;
	}

	if (isOpen()) {
		qDebug() << "SQLDatabase::open: database was already open, closing first";

		close();
	}

	mConnectionName = connName;

	qDebug() << "SQLDatabase::open: Trying to open database with connection name" << mConnectionName << "and driver" << mType;

	QSqlDatabase db = QSqlDatabase::addDatabase(mType, mConnectionName);

	if (db.isValid()) {
		if (dbnameOnly) {
			// for SQLite-like business
			db.setHostName("localhost");
			db.setDatabaseName(dbname);
		}
		else {
			// normal SQL RDBMS
			db.setHostName(host);
			db.setPort(port);

			db.setDatabaseName(dbname);
			db.setUserName(user);
			db.setPassword(pass);
		}

		setConnectOptions();

		if (db.open()) {
			if (!hasCorrectCapabilities()) {
				qDebug() << "SQLDatabase::open:" << mType << "Did not have all the correct capabilities, certain methods may fail";
			}

			setPragmas();

			if (!tables().contains("matches")) {
				qDebug() << "SQLDatabase::open: database opened correctly but was found to be empty, setting up Thera schema";

				setup(SCHEMA_FILE);
			}
			else {
				qDebug() << "SQLDatabase::open: database opened correctly and already contained tables:\n\t" << tables();

				emit matchFieldsChanged();
			}

			// not necessary, will trigger on matchFieldsChanged() anyway
			// if (mTrackHistory) createHistory();

			// the order is actually important, because for example the models react to matchCountChanged, but matchFieldsChanged needs to have ran by then
			emit databaseOpened();
			emit matchCountChanged();

			return true;
		}
		else {
			qDebug() << "SQLDatabase::open: Could not open connection to database:" << db.lastError();;
		}
	}
	else {
		qDebug() << QString("SQLDatabase::open: Connection to database was invalid, driver = %1, connection name = %2").arg(mType).arg(mConnectionName);
	}

	return false;
}

// this method assumes every paramater for connection has already been set AND that the database has
// been set up properly at least once (i.e.: it is dumb and for internal use)
// return true for success and false for failure
bool SQLDatabase::reopen() {
	return QSqlDatabase::database(mConnectionName, true).isOpen();
}

QString SQLDatabase::connectionName() const {
	return database().connectionName();
}

SQLDatabase::SQLDatabase(QObject *parent, const QString& type, bool trackHistory)
	: QObject(parent), mType(type), mTrackHistory(trackHistory) {
	//QObject::connect(this, SIGNAL(databaseClosed()), this, SLOT(resetQueries()));
	QObject::connect(this, SIGNAL(matchFieldsChanged()), this, SLOT(makeFieldsSet()));
	QObject::connect(this, SIGNAL(matchFieldsChanged()), this, SLOT(createHistory()));
}

SQLDatabase::~SQLDatabase() {
	// copying and assigning are allowed now, we'd have to reference count OR rely on some master
	// object calling close() for us
	// TODO: re-evaluate this decision

	qDebug() << "SQLDatabase::~SQLDatabase:" << connectionName() << "running, database is currently still" << (isOpen() ? "open" : "closed");

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

bool SQLDatabase::detectClosedDb() const {
	// TODO: build real detection code for MySQL (i.e.: prepare a statement and see if it errors out)
	return !isOpen();
}

// the default implementation does nothing
QString SQLDatabase::makeCompatible(const QString& statement) const {
	return statement;
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

/**
 * This code will work for most SQL databases, SQLite is an exception though,
 * which is why this code is overriden in that specific sublass
 */
QStringList SQLDatabase::tables(QSql::TableType type) const {
	QStringList list;

	if (!isOpen()) return list;

	QString typeSelector = "AND TABLE_TYPE = '%1'";

	switch (type) {
		case QSql::Tables:
			typeSelector = typeSelector.arg("BASE TABLE");
			break;

		case QSql::Views:
			typeSelector = typeSelector.arg("VIEW");
			break;

		case QSql::AllTables:
			typeSelector = QString();
			break;

		default:
			qDebug() << "SQLDatabase::tables: unknown option (type)" << type;

			return list;
			break;
	}

	QSqlDatabase db = database();
	QString queryString = QString("SELECT TABLE_NAME, TABLE_TYPE FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = '%1' %2;")
			.arg(db.databaseName())
			.arg(typeSelector);
	QSqlQuery query(db);
	query.setForwardOnly(true);
	if (query.exec(queryString)) {
		while (query.next()) {
			list << query.value(0).toString();
		}
	}
	else {
		qDebug() << "SQLDatabase::tables: query error" << query.lastError()
			<< "\nQuery executed:" << query.lastQuery();
	}

	return list;
}

bool SQLDatabase::transaction() const {
	return database().transaction();
}

bool SQLDatabase::commit() const {
	return database().commit();
}

void SQLDatabase::createIndex(const QString& table, const QStringList& fields) {
	QSqlQuery query(database());
	if (query.exec(QString("CREATE INDEX %3_index ON %1(%2);").arg(table).arg(fields.join(",")).arg(fields.join("_")))) {
		qDebug() << "SQLDatabase::createIndex: successfully created index on field(s)" << fields << "on table" << table;
	}
	else {
		qDebug() << "SQLDatabase::createIndex: failed creating index" << fields << "on" << table << "->" << query.lastError();
	}
}

thera::SQLFragmentConf SQLDatabase::addMatch(const QString& sourceName, const QString& targetName, const thera::XF& xf, int id) {
	const QString queryKey = (id == -1) ? "addMatchNoId" : "addMatchWithId";
	const QString queryString = (id == -1)
			?
			"INSERT INTO matches (source_id, source_name, target_id, target_name, transformation) "
			"VALUES (:source_id, :source_name, :target_id, :target_name, :transformation)"
			:
			"INSERT INTO matches (match_id, source_id, source_name, target_id, target_name, transformation) "
			"VALUES (:match_id, :source_id, :source_name, :target_id, :target_name, :transformation)";

	QSqlQuery &query = getOrElse(queryKey, queryString);

	QString xfs;

	for (int col = 0; col < 4; ++col) {
		for (int row = 0; row < 4; ++row) {
			xfs += QString("%1 ").arg(xf[4 * row + col], 0, 'e', 20);
		}
	}

	if (id != -1) query.bindValue(":match_id", id);
	query.bindValue(":source_id", 0); // TODO: not use dummy value
	query.bindValue(":source_name", sourceName);
	query.bindValue(":target_id", 0); // TODO: not use dummy value
	query.bindValue(":target_name", targetName);
	query.bindValue(":transformation", xfs);

	SQLDatabase *db = NULL;
	int realId = -1;
	int fragments[IFragmentConf::MAX_FRAGMENTS];

	if (query.exec()) {
		db = this;
		realId = query.lastInsertId().toInt();

		fragments[IFragmentConf::SOURCE] = Database::entryIndex(sourceName);
		fragments[IFragmentConf::TARGET] = Database::entryIndex(targetName);
	}
	else {
		qDebug() << "SQLDatabase::addMatch: could not insert match record, returning invalid SQLFragmentConf:" << query.lastError();

		fragments[IFragmentConf::SOURCE] = -1;
		fragments[IFragmentConf::TARGET] = -1;
	}

	if (id != -1 && realId != id) {
		qDebug() << "SQLDatabase::addMatch: the inserted id was valid but differed from the requested id. Got " << realId << " as opposed to requested id" << id
			<< "\n\tqueryKey =" << queryKey
			<< "\n\t" << query.lastQuery()
			<< "\n\t" << query.boundValues();
	}

	return SQLFragmentConf(db, realId, fragments, 1.0f, xf);
}

QSet<SQLDatabase::SpecialCapabilities> SQLDatabase::supportedCapabilities() const { return QSet<SpecialCapabilities>(); }
bool SQLDatabase::supports(SpecialCapabilities) const { return false; }

void SQLDatabase::setConnectOptions() const {
	// the default is no connection options
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

			qDebug() << "SQLDatabase::loadFromXML: Starting to parse XML";

			parseXML(root);

			qDebug() << "SQLDatabase::loadFromXML: Done parsing XML, adding extra attributes:";

			addMatchField("comment", "");
			addMatchField("duplicate", 0);
			addMetaMatchField("num_duplicates", "SELECT duplicate AS match_id, COUNT(duplicate) AS num_duplicates FROM duplicate GROUP BY duplicate");

			//emit matchFieldsChanged(); <--- in general already called by the addMatchField calls (takes care of available attributes management and history creation)
			emit matchCountChanged();

			qDebug() << "SQLDatabase::loadFromXML: Done adding extra attributes, hopefully nothing went wrong";
		}
		else {
			qDebug() << "Reading XML file" << XMLFile << "failed";
		}
	}
	else {
		qDebug() << "Could not open"  << XMLFile;
	}
}

void SQLDatabase::stressTestFromXML(const QString& XMLFile, int factor, bool perturb) {
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

			qDebug() << "SQLDatabase::stressTestFromXML: Starting to parse XML";

			parseXMLStressTest(root, factor, perturb);

			qDebug() << "SQLDatabase::stressTestFromXML: Done parsing XML, adding extra attributes:";

			addMatchField("comment", "");
			addMatchField("duplicate", 0);
			addMetaMatchField("num_duplicates", "SELECT duplicate AS match_id, COUNT(duplicate) AS num_duplicates FROM duplicate GROUP BY duplicate");

			//emit matchFieldsChanged(); <--- in general already called by the addMatchField calls (takes care of available attributes management and history creation)
			emit matchCountChanged();

			qDebug() << "SQLDatabase::stressTestFromXML: Done adding extra attributes, hopefully nothing went wrong";
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
	if (addMatchField(name, "REAL", defaultValue)) {
		emit matchFieldsChanged();

		return true;
	}

	return false;
}

bool SQLDatabase::addMatchField(const QString& name, const QString& defaultValue) {
	if (addMatchField(name, "TEXT", defaultValue)) {
		emit matchFieldsChanged();

		return true;
	}

	return false;
}

bool SQLDatabase::addMatchField(const QString& name, int defaultValue) {
	if (addMatchField(name, "INTEGER", defaultValue)) {
		emit matchFieldsChanged();

		return true;
	}

	return false;
}

template<typename T> bool SQLDatabase::addMatchField(const QString& name, const QString& sqlType, T defaultValue, bool indexValue) {
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

	if (!transaction()) {
		qDebug() << "SQLDatabase::addMatchField: could NOT start a transaction, the following might be very slow";
	}

	/*
	if (query.exec("START TRANSACTION")) qDebug() << "SQLDatabase::addMatchField: Correctly manually started a transaction";
	else qDebug() << "SQLDatabase::addMatchField can't start transaction:" << query.lastError();

	if (query.exec("BEGIN")) qDebug() << "SQLDatabase::addMatchField: Correctly manually started a transaction";
	else qDebug() << "SQLDatabase::addMatchField can't start transaction:" << query.lastError();
	*/

	success = query.exec(QString("CREATE TABLE %1 (match_id INTEGER PRIMARY KEY, %1 %2, confidence REAL)").arg(name).arg(sqlType));
	if (success) {
		// insert the default value everywhere
		query.prepare(QString(
			"INSERT INTO %1 (match_id, %1, confidence) "
			"VALUES (:match_id, :value, :confidence)"
		).arg(name));

		//int i = 0;
		//int step = 100;
		QElapsedTimer timer;
		timer.start();

		QSqlQuery idQuery(db);
		query.setForwardOnly(true);
		if (idQuery.exec("SELECT match_id FROM matches")) {
			qDebug() << "SQLDatabase::addMatchField: Fetched all matches in" << timer.restart() << "msec";

			while (idQuery.next()) {
				query.bindValue(":match_id", idQuery.value(0).toInt());
				query.bindValue(":value", defaultValue);
				query.bindValue(":confidence", 1.0);

				query.exec();

				/*
				if (++i % step == 0) {
					qDebug() << "Inserted another" << step << "rows for field" << name << "now at" << i << "used" << timer.restart() << "msec";
				}
				*/
			}
		}
		else {
			qDebug() << "SQLDatabase::addMatchField couldn't create default values:" << idQuery.lastError()
				<< "\nQuery executed:" << idQuery.lastQuery();
		}

		if (indexValue) {
			createIndex(name, QStringList() << name);
		}

		qDebug() << "SQLDatabase::addMatchField succesfully created field:" << name;

		//emit matchFieldsChanged();
	}
	else {
		qDebug() << "SQLDatabase::addMatchField couldn't create table:" << query.lastError()
			<< "\nQuery executed:" << query.lastQuery();
	}

	commit();

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

	transaction();
	success = query.exec(createViewQuery(name, sql));
	//success = query.exec(QString("CREATE VIEW IF NOT EXISTS %1 AS %2").arg(name).arg(sql));
	if (success) {
		qDebug() << "SQLDatabase::addMetaMatchField: Create view appears to have been succesful, query:" << query.lastQuery();
		emit matchFieldsChanged();
	}
	else {
		qDebug() << "SQLDatabase::addMetaMatchField: couldn't create VIEW table:" << query.lastError()
			<< "\nQuery executed:" << query.lastQuery();
	}
	commit();

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

	transaction();
	if (!query.exec(queryString)) {
		qDebug() << "SQLDatabase::removeMatchField couldn't drop table:" << query.lastError()
				<< "\nQuery executed:" << query.lastQuery();
	}
	else {
		emit matchFieldsChanged();
	}
	commit();

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

thera::SQLFragmentConf SQLDatabase::getMatch(int id) {
	const QString queryString = QString("SELECT matches.match_id, source_name, target_name, transformation FROM matches WHERE match_id = %1").arg(id);

	int matchId = -1;
	SQLDatabase *db = NULL;
	int fragments[IFragmentConf::MAX_FRAGMENTS];
	XF xf;

	QSqlQuery query(database());
	if (query.exec(queryString) && query.first()) {
		db = this;
		matchId = query.value(0).toInt();

		assert(matchId == id);

		QTextStream ts(query.value(3).toString().toAscii());
		ts >> xf;

		fragments[IFragmentConf::SOURCE] = Database::entryIndex(query.value(1).toString());
		fragments[IFragmentConf::TARGET] = Database::entryIndex(query.value(2).toString());
	}
	else {
		qDebug() << "SQLDatabase::getNumberOfMatches: problem with query:" << query.lastError();
	}

	return SQLFragmentConf(db, matchId, fragments, 1.0f, xf);
}

QList<thera::SQLFragmentConf> SQLDatabase::getMatches(const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int offset, int limit) {
	QString queryString = synthesizeQuery(QStringList(), sortField, order, filter, offset, limit);

	return fillFragments(queryString, QStringList());
}

QList<thera::SQLFragmentConf> SQLDatabase::getPreloadedMatches(const QStringList& _preloadFields, const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int offset, int limit) {
	if (_preloadFields.isEmpty()) return getMatches(sortField, order, filter, offset, limit);

	QSet<QString> dependencies = filter.dependencies().toSet();

	// if there are no VIEW's as sort fields or as dependencies, we can make the query quite a lot faster by forcing a certain order of evaluation (mostly MySQL)
	if (!(_preloadFields.toSet() & mViewMatchFields).isEmpty() && !mViewMatchFields.contains(sortField) && (dependencies & mViewMatchFields).isEmpty()) return getPreloadedMatchesFast(_preloadFields, sortField, order, filter, offset, limit);

	QString queryString = synthesizeQuery(_preloadFields, sortField, order, filter, offset, limit);
	return fillFragments(queryString, _preloadFields);
}

QList<thera::SQLFragmentConf> SQLDatabase::getFastPaginatedPreloadedMatches(const QStringList& _preloadFields, const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int limit, int extremeMatchId, double extremeSortValue, bool forward, bool inclusive, int offset) {
	//if (_preloadFields.isEmpty()) return getMatches(sortField, order, filter, offset, limit);

	//QSet<QString> dependencies = filter.dependencies().toSet();

	// if there are no VIEW's as sort fields or as dependencies, we can make the query quite a lot faster by forcing a certain order of evaluation (mostly MySQL)
	//if (!(_preloadFields.toSet() & mViewMatchFields).isEmpty() && !mViewMatchFields.contains(sortField) && (dependencies & mViewMatchFields).isEmpty()) return getPreloadedMatchesFast(_preloadFields, sortField, order, filter, offset, limit);

	QString queryString = synthesizeFastPaginatedQuery(_preloadFields, sortField, order, filter, limit, extremeMatchId, extremeSortValue, forward, inclusive, offset);

	QList<thera::SQLFragmentConf> list = fillFragments(queryString, _preloadFields);

	if (!forward) {
		for (int k = 0; k < (list.size() / 2); ++k) {
			list.swap(k,list.size()-(1+k));
		}
	}

	return list;
}

/**
 * This is definitely much faster for MySQL when VIEW-joins are at play, doesn't hurt SQLite either (other DB systems untested)
 *
 * @pre
 * 		Neither the sortField nor any dependency of the filter is a meta-attribute/view (but one of the preloadFields can be)
 *
 * 	The inspiration:

	DROP VIEW IF EXISTS `matches_joined`;

	CREATE VIEW `matches_joined` AS (
	  SELECT matches.match_id, source_name, target_name, transformation, STATUS , volume, error, COMMENT
	  FROM matches
	  INNER JOIN STATUS ON matches.match_id = status.match_id
	  INNER JOIN error ON matches.match_id = error.match_id
	  INNER JOIN volume ON matches.match_id = volume.match_id
	  INNER JOIN COMMENT ON matches.match_id = comment.match_id
	  ORDER BY error.error
	LIMIT 20, 20
	);.

	SELECT matches_joined.*, num_duplicates
	FROM matches_joined
	LEFT JOIN num_duplicates ON matches_joined.match_id = num_duplicates.match_id
 */
QList<thera::SQLFragmentConf> SQLDatabase::getPreloadedMatchesFast(const QStringList& _preloadFields, const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int offset, int limit) {
	// maybe put some ASSERT's here to check the preconditions... (performance...)
	assert(!mViewMatchFields.contains(sortField));
	assert((mViewMatchFields & filter.dependencies().toSet()).isEmpty());

	// remove all VIEW tables from the preloadFields
	QSet<QString> preloadFieldsSet = _preloadFields.toSet();
	QStringList preloadFields = (preloadFieldsSet - mViewMatchFields).toList();
	QStringList preloadMetaFields = (preloadFieldsSet - mNormalMatchFields).toList();

	if ((preloadFieldsSet & mViewMatchFields).isEmpty()) qDebug() << "SQLDatabase::getPreloadedMatchesFast: the fast path was wrongly chosen, no meta-attribute needs to be retrieved";

	QString queryString = synthesizeQuery(preloadFields, sortField, order, filter, offset, limit);

	QElapsedTimer timer;
	timer.start();

	qint64 viewCreateTime = 0;

	QSqlQuery q(database());
	if (!q.exec("DROP VIEW IF EXISTS `matches_joined_temp`;")) {
		qDebug() << "SQLDatabase::getPreloadedMatchesFast: couldn't drop view:" << q.lastError();
	}
	if (!q.exec(createViewQuery("matches_joined_temp", queryString))) {
		qDebug() << "SQLDatabase::getPreloadedMatchesFast: couldn't create view:" << q.lastError()
			<< "\n\tQUERY =" << q.lastQuery();
	}

	viewCreateTime = timer.elapsed();
	qDebug() << "SQLDatabase::getPreloadedMatchesFast: QUERY =" << queryString << "\n\tcreating view took" << viewCreateTime << "msec";

	// query the newly created VIEW instead of the real tables
	queryString = QString("SELECT matches_joined_temp.*, %1 FROM `matches_joined_temp`").arg(preloadMetaFields.join(", "));

	foreach (const QString& field, preloadMetaFields) {
		queryString += QString(" LEFT JOIN %1 ON matches_joined_temp.match_id = %1.match_id").arg(field);
	}

	return fillFragments(queryString, _preloadFields);
}

QString SQLDatabase::synthesizeQuery(const QStringList& _requiredFields, const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int offset, int limit) const {
	QSet<QString> dependencies = filter.dependencies().toSet();

	QString queryString;

	QStringList requiredFields = _requiredFields;

	// add all valid required fields to the dependencies and filter out the ones that don't exist
	foreach (const QString& field, requiredFields) {
		if (matchHasField(field)) {
			dependencies << field;
		}
		else {
			requiredFields.removeOne(field);
		}
	}

	if (requiredFields.size() != _requiredFields.size()) {
		qDebug() << "SQLDatabase::synthesizeQuery: One of the requested attributes was not available in the database:" << requiredFields << "->" << _requiredFields
			<< "\n\tAll available attributes are:" << matchFields();
	}

	if (requiredFields.isEmpty()) {
		queryString = QString("SELECT matches.match_id, source_name, target_name, transformation FROM matches");
	}
	else {
		queryString = QString("SELECT matches.match_id, source_name, target_name, transformation, %1 FROM matches").arg(requiredFields.join(","));
	}

	if (!sortField.isEmpty()) {
		if (matchHasField(sortField)) dependencies << sortField;
		else qDebug() << "SQLDatabase::synthesizeQuery: attempted to sort on field" << sortField << "which doesn't exist";
	}

	//join in dependencies
	foreach (const QString& field, dependencies) {
		if (mViewMatchFields.contains(field)) {
			queryString += QString(" LEFT JOIN %1 ON matches.match_id = %1.match_id").arg(field);
		}
		else {
			// if the JOIN table is also the on we're sorting on, it's usually advantageous to let MySQL know
			// that we'd appreciate it if it used that tables index instead of anything else. This saves a temporary table and a filesort
			QString force = (field == sortField && supports(FORCE_INDEX_MYSQL)) ? QString(" FORCE INDEX (%1_index) ").arg(field) : QString(" ");

			queryString += QString(" INNER JOIN %1%2ON matches.match_id = %1.match_id").arg(field).arg(force);
		}
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

	return queryString;
}

/**
 * Pagination optimizations... quite experimental
 *
 * if going backwards this will put all the fragments in reverse order (for now)
 */
QString SQLDatabase::synthesizeFastPaginatedQuery(
		const QStringList& _requiredFields,
		const QString& sortField,
		Qt::SortOrder order,
		const SQLFilter& filter,
		int limit, int extremeMatchId, double extremeSortValue, bool forward, bool inclusive, int offset) const {
	QSet<QString> dependencies = filter.dependencies().toSet();

	QString queryString;

	QStringList requiredFields = _requiredFields;

	// add all valid required fields to the dependencies and filter out the ones that don't exist
	foreach (const QString& field, requiredFields) {
		if (matchHasField(field)) {
			dependencies << field;
		}
		else {
			requiredFields.removeOne(field);
		}
	}

	if (requiredFields.size() != _requiredFields.size()) {
		qDebug() << "SQLDatabase::synthesizeQuery: One of the requested attributes was not available in the database:" << requiredFields << "->" << _requiredFields
			<< "\n\tAll available attributes are:" << matchFields();
	}

	if (requiredFields.isEmpty()) {
		queryString = QString("SELECT matches.match_id, source_name, target_name, transformation FROM matches");
	}
	else {
		queryString = QString("SELECT matches.match_id, source_name, target_name, transformation, %1 FROM matches").arg(requiredFields.join(","));
	}

	if (!sortField.isEmpty()) {
		if (matchHasField(sortField)) dependencies << sortField;
		else qDebug() << "SQLDatabase::synthesizeQuery: attempted to sort on field" << sortField << "which doesn't exist";
	}

	//join in dependencies
	foreach (const QString& field, dependencies) {
		if (mViewMatchFields.contains(field)) {
			queryString += QString(" LEFT JOIN %1 ON matches.match_id = %1.match_id").arg(field);
		}
		else {
			// if the JOIN table is also the on we're sorting on, it's usually advantageous to let MySQL know
			// that we'd appreciate it if it used that tables index instead of anything else. This saves a temporary table and a filesort
			QString force = (field == sortField && supports(FORCE_INDEX_MYSQL)) ? QString(" FORCE INDEX (%1_index) ").arg(field) : QString(" ");

			queryString += QString(" INNER JOIN %1%2ON matches.match_id = %1.match_id").arg(field).arg(force);
		}
	}

	// this is where the fast pagination magic happens
	//QString op = (order == Qt::AscendingOrder && forward || order == Qt::DescendingOrder && !forward) ? ">" : "<";

	QString realSortOrder = "DESC";
	QString op = "<";

	if ((order == Qt::AscendingOrder && forward) || (order == Qt::DescendingOrder && !forward)) {
		realSortOrder = "ASC";
		op = ">";
	}

	// only if sortField is not empty
	QString valueClamper = QString(" WHERE (%1.%1 %3= %2) AND (%1.match_id %3%5 %4 OR %1.%1 %3 %2)").arg(sortField).arg(extremeSortValue).arg(op).arg(extremeMatchId).arg(inclusive ? "=" : "");

	queryString += valueClamper;

	/*
	// going to a next page if initially ascending
	WHERE (
		error.error >= 0.0408032
	) AND (
		error.match_id >37432
		OR error.error > 0.0408032
	)
		ORDER BY error.error ASC , error.match_id ASC
		LIMIT 20
	*/

	// add filter clauses
	if (!filter.isEmpty()) {
		queryString += " AND (" + filter.clauses().join(") AND (") + ")";
	}

	if (!sortField.isEmpty()) {
		queryString += QString(" ORDER BY %1.%1 %2, %1.match_id %2").arg(sortField).arg(realSortOrder);
	}

	if (offset != -1 && limit != -1) {
		queryString += QString(" LIMIT %1, %2").arg(offset).arg(limit);
	}

	return queryString;
}

QList<thera::SQLFragmentConf> SQLDatabase::fillFragments(const QString& queryString, const QStringList& cacheFields) {
	QElapsedTimer timer;
	timer.start();

	qint64 queryTime = 0, fillTime = 0;

	QList<SQLFragmentConf> list;

	int fragments[IFragmentConf::MAX_FRAGMENTS];
	XF xf;

	QSqlQuery query(database());
	query.setForwardOnly(true);

	if (query.exec(queryString)) {
		QSqlRecord rec = query.record();

		typedef QPair<QString, int> StringIntPair;
		QList<StringIntPair> fieldIndexList;
		foreach (const QString& field, cacheFields) {
			//qDebug() << "Adding preload field for caching:" << StringIntPair(field, rec.indexOf(field));
			fieldIndexList << StringIntPair(field, rec.indexOf(field));
		}

		queryTime = timer.restart();

		while (query.next()) {
			fragments[IFragmentConf::SOURCE] = Database::entryIndex(query.value(1).toString());
			fragments[IFragmentConf::TARGET] = Database::entryIndex(query.value(2).toString());

			QMap<QString, QVariant> cache;
			foreach (const StringIntPair& pair, fieldIndexList) {
				//qDebug() << "Caching fastpath: " << pair << "for id" << query.value(0).toInt();

				cache.insert(pair.first, query.value(pair.second));
			}

			QTextStream ts(query.value(3).toString().toAscii());
			ts >> xf;

			list << SQLFragmentConf(this, cache, query.value(0).toInt(), fragments, 1.0f, xf);
		}
	}
	else {
		qDebug() << "SQLDatabase::getMatches query failed:" << query.lastError()
				<< "\nQuery executed:" << query.lastQuery();
	}

	fillTime = timer.elapsed();
	qDebug() << "SQLDatabase::getMatches: QUERY =" << queryString << "\n\tquery took" << queryTime << "msec and filling the list took" << fillTime << "msec (filled" << list.size() << "SQLFragmentConf's)";

	return list;
}

bool SQLDatabase::historyAvailable() const {
	return mTrackHistory;
}

QList<HistoryRecord> SQLDatabase::getHistory(const QString& field, const QString& sortField, Qt::SortOrder order, const SQLFilter& filter, int offset, int limit) {
	QList<HistoryRecord> list;

	if (!matchHasField(field)) {
		qDebug() << "SQLDatabase::getHistory: field" << field << "did not exist";

		return list;
	}

	QString queryString = QString("SELECT user_id, match_id, timestamp, %1 FROM %1_history").arg(field);

	QSet<QString> historyFields = QSet<QString>() << "match_id" << "user_id" << "timestamp" << field;
	if (!sortField.isEmpty()) {
		if (!historyFields.contains(sortField)) {
			qDebug() << "SQLDatabase::getHistory: attempted to sort on field" << sortField << "which doesn't exist";
		}
	}

	// add filter clauses
	/*
	if (!filter.isEmpty()) {
		queryString += " WHERE (" + filter.clauses().join(") AND (") + ")";
	}
	*/

	if (!sortField.isEmpty()) {
		queryString += QString(" ORDER BY %1 %2").arg(sortField).arg(order == Qt::AscendingOrder ? "ASC" : "DESC");
	}

	QSqlQuery query(database());
	query.setForwardOnly(true);

	if (query.exec(queryString)) {
		while (query.next()) {
			list << HistoryRecord(
				query.value(0).toInt(),
				query.value(1).toInt(),
				QDateTime::fromTime_t(query.value(2).toUInt()),
				query.value(3)
			);
		}
	}
	else {
		qDebug() << "SQLDatabase::getHistory query failed:" << query.lastError()
			<< "\nQuery executed:" << query.lastQuery();
	}

	return list;
}

QList<AttributeRecord> SQLDatabase::getAttribute(const QString& field) {
	QList<AttributeRecord> list;

	// TODO: obviously...

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

QString SQLDatabase::escapeCharacter() const {
	return QString();
}

int SQLDatabase::matchCount() const {
	if (!isOpen()) {
		return 0;
	}

	QSqlQuery query(database());

	if (query.exec("SELECT Count(match_id) FROM matches") && query.first()) {
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

	QSqlDatabase db(database());

	QStringList integerAttributes = QStringList() << "status";
	QStringList floatAttributes = QStringList() << "error" << "overlap" << "volume" << "old_volume" << "probability";
	QStringList stringAttributes = QStringList();

	// create the attribute tables if they don't exist
	foreach (const QString& attr, integerAttributes) {
		if (!matchHasRealField(attr)) addMatchField(attr, "INTEGER", "0");
	}

	foreach (const QString& attr, floatAttributes) {
		if (!matchHasRealField(attr)) addMatchField(attr, "REAL", "0");
	}

	foreach (const QString& attr, stringAttributes) {
		if (!matchHasRealField(attr)) addMatchField(attr, "TEXT", "0");
	}

	transaction();

	// prepare queries
	QSqlQuery matchesQuery(db);
	matchesQuery.prepare(
		"INSERT INTO matches (match_id, source_name,target_name, transformation) "
		"VALUES (:match_id, :source_name, :target_name, :transformation)"
	);
	/*
	matchesQuery.prepare(
		"INSERT INTO matches (match_id, source_id, source_name, target_id, target_name, transformation) "
		"VALUES (:match_id, :source_id, :source_name, :target_id, :target_name, :transformation)"
	);
	*/

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

	QSqlQuery probabilityQuery(db);
	probabilityQuery.prepare(
		"INSERT INTO probability (match_id, probability) "
		"VALUES (:match_id, :probability)"
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
		//matchesQuery.bindValue(":source_id", 0); // TODO: not use dummy value
		matchesQuery.bindValue(":source_name", match.attribute("src"));
		//matchesQuery.bindValue(":target_id", 0); // TODO: not use dummy value
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

		/*
		static int j = 0;
		if (j++ == 0) {
			for (int k = 0; k < match.attributes().length(); ++k)
				qDebug() << match.attributes().item(k).toText().data();
		}
		*/

		// case sensitive!
		if (match.hasAttribute("Probability")) {
			probabilityQuery.bindValue(":match_id", matchId);
			probabilityQuery.bindValue(":probability", match.attribute("Probability", "0.0").toDouble());
			probabilityQuery.exec();
		}

		emit databaseOpStepDone(i);

		++i;
	}

	commit();

	emit databaseOpEnded();
}

void SQLDatabase::parseXMLStressTest(const QDomElement &root, int factor, bool perturb) {
	QSqlDatabase db(database());

	QStringList integerAttributes = QStringList() << "status";
	QStringList floatAttributes = QStringList() << "error" << "overlap" << "volume" << "old_volume" << "probability";
	QStringList stringAttributes = QStringList();

	// create the attribute tables if they don't exist
	foreach (const QString& attr, integerAttributes) {
		if (!matchHasRealField(attr)) addMatchField(attr, "INTEGER", "0");
	}

	foreach (const QString& attr, floatAttributes) {
		if (!matchHasRealField(attr)) addMatchField(attr, "REAL", "0");
	}

	foreach (const QString& attr, stringAttributes) {
		if (!matchHasRealField(attr)) addMatchField(attr, "TEXT", "");
	}

	transaction();

	// prepare queries
	QSqlQuery matchesQuery(db);
	matchesQuery.prepare(
		"INSERT INTO matches (match_id, source_name,target_name, transformation) "
		"VALUES (:match_id, :source_name, :target_name, :transformation)"
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

	QSqlQuery probabilityQuery(db);
	probabilityQuery.prepare(
		"INSERT INTO probability (match_id, probability) "
		"VALUES (:match_id, :probability)"
	);

	emit databaseOpStarted(tr("Converting XML file to database"), root.childNodes().length());

	int i = 0;
	int idcounter = 0;

	for (QDomElement match = root.firstChildElement("match"); !match.isNull(); match = match.nextSiblingElement()) {
		//int matchId = match.attribute("id").toInt();
		QString rawTransformation(match.attribute("xf", "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1").toAscii());

		bool hasProb = match.hasAttribute("Probability");

		QString source = match.attribute("src");
		QString target = match.attribute("tgt");
		int status =  match.attribute("status", "0").toInt();
		float error =  match.attribute("error", "0").toFloat();
		float overlap =  match.attribute("overlap", "0").toFloat();
		float volume =  match.attribute("volume", "0").toFloat();
		float old_volume =  match.attribute("old_volume", "0").toFloat();
		float probability = match.attribute("Probability", "0.0").toFloat();

		for (int j = 0; j < factor; ++j, ++idcounter) {
			matchesQuery.bindValue(":match_id", idcounter);
			matchesQuery.bindValue(":source_name", source);
			matchesQuery.bindValue(":target_name", target);
			matchesQuery.bindValue(":transformation", rawTransformation);
			matchesQuery.exec();

			// update attribute tables
			statusQuery.bindValue(":match_id", idcounter);
			statusQuery.bindValue(":status", (status + qrand()) % 5);
			statusQuery.exec();

			errorQuery.bindValue(":match_id", idcounter);
			errorQuery.bindValue(":error", error + (perturb && j != 0) ? (float(rand()) / float(RAND_MAX)) : 0);
			errorQuery.exec();

			overlapQuery.bindValue(":match_id", idcounter);
			overlapQuery.bindValue(":overlap", overlap + (perturb && j != 0) ? (float(rand()) / float(RAND_MAX)) : 0);
			overlapQuery.exec();

			volumeQuery.bindValue(":match_id", idcounter);
			volumeQuery.bindValue(":volume", volume + (perturb && j != 0) ? (float(rand()) / float(RAND_MAX)) : 0);
			volumeQuery.exec();

			old_volumeQuery.bindValue(":match_id", idcounter);
			old_volumeQuery.bindValue(":old_volume", old_volume + (perturb && j != 0) ? (float(rand()) / float(RAND_MAX)) : 0);
			old_volumeQuery.exec();

			if (hasProb) {
				probabilityQuery.bindValue(":match_id", idcounter);
				probabilityQuery.bindValue(":probability", probability + (perturb && j != 0) ? (float(rand()) / float(RAND_MAX)) : 0);
				probabilityQuery.exec();
			}
		}

		emit databaseOpStepDone(i);

		++i;
	}

	commit();

	emit databaseOpEnded();
	//emit matchCountChanged();
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

	transaction();
	foreach (const QString &q, queries) {
		query.exec(q);
		qDebug() << "Executed query:" << q;
	}
	commit();

	emit matchFieldsChanged();
}

void SQLDatabase::createHistory() {
	if (!isOpen()) return;
	if (!mTrackHistory) return;

	QSqlDatabase db = database();
	QSqlQuery query(db);

	// optionally remove the history tables first (this would constitute a reset)
	//foreach (const QString& field, mNormalMatchFields)
	//	if (!query.exec(QString("DROP TABLE %1_history").arg(field))) qDebug() << "Couldn't remove history table for" << field << "error:" << query.lastError();

	// retrieve ALL normal (== non-view) tables in the current database
	QStringList t = tables();
	QStringList created;
	QStringList kept;

	// this will by now contain all loaded fields, for each field create a history table if none exists
	foreach (const QString& field, mNormalMatchFields) {
		QString fieldHistoryTable = field + "_history";

		if (!t.contains(fieldHistoryTable)) {
			createHistory(field);

			created << field;

			// perhaps fill with initial data (all current values with the user included)

			//https://dev.mysql.com/doc/refman/5.1/en/create-table-select.html
			//http://stackoverflow.com/questions/4007014/alter-column-in-sqlite
		}
		else {
			kept << field;
		}
	}

	if (!created.isEmpty()) qDebug() << "SQLDatabase::createHistory: history created for fields" << created;
	if (!kept.isEmpty()) qDebug() << "SQLDatabase::createHistory: history already existed for fields" << kept;
}

// generic method that should work for most SQL db's (doesn't work for SQLite so reimplemented in that specific sublass)
void SQLDatabase::createHistory(const QString& table) {
	QSqlQuery query(database());

	if (query.exec(QString("CREATE TABLE %1_history (user_id INT, timestamp INT) AS (SELECT * FROM %1 WHERE 1=2);").arg(table))) {
		qDebug() << "SQLDatabase::createHistory: succesfully created history for" << table;
	}
	else {
		qDebug() << "SQLDatabase::createHistory: couldn't create history table for" << table << "->" << query.lastError() << "\n\tExecuted:" << query.lastQuery();
	}
}


void SQLDatabase::close() {
	// resource cleanup in any case, after this function is done we should be 100% sure that the database is closed and the resources are cleaned up
	resetQueries();

	if (isOpen()) {
		qDebug() << "SQLDatabase::close: Closing database with connection name" << database().connectionName();

		database().close();
		QSqlDatabase::removeDatabase(mConnectionName);

		emit databaseClosed();
	}
	else {
		qDebug() << "SQLDatabase::close: Couldn't close current database" << database().connectionName() << "because it wasn't open to begin with";
	}
}

void SQLDatabase::setConnectionName(const QString& connectionName) {
	mConnectionName = connectionName;
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
	mNormalMatchFields.clear();
	mViewMatchFields.clear();

	foreach (const QString& table, tables()) {
		// check if the tables name is not the 'matches' table itself
		if (table != "matches") {
			// check if the table contains a match_id attribute
			QSet<QString> fields = tableFields(table);
			if (fields.contains("match_id") && fields.contains(table)) {
				mNormalMatchFields << table;
				mMatchFields << table;
			}
		}
	}

	// include the view attributes as well but add them to mViewMatchFields as well so we can differentiate them later
	// from the normal ones
	foreach (const QString& table, tables(QSql::Views)) {
		// check if the tables name is not the 'matches' table itself
		if (table != "matches") {
			QSet<QString> fields = tableFields(table);
			if (fields.contains("match_id") && fields.contains(table)) {
				mViewMatchFields << table;
				mMatchFields << table;
			}
		}
	}

	// add the default attributs that are special and always there (their "special" status may dissapear later though)
	//mMatchFields << "source_id" << "source_name" << "target_id" << "target_name" << "transformation";
}
