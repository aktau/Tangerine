#include "SQLDatabase.h"

#include <QFile>
#include <QTextStream>

#include "XF.h"

#include "SQLiteDatabase.h"
#include "SQLMySqlDatabase.h"
#include "SQLNullDatabase.h"

#include "SQLConnectionDescription.h"

using namespace thera;

QHash<QString, QWeakPointer<SQLDatabase> > SQLDatabase::mActiveConnections;

const QString SQLDatabase::SCHEMA_FILE = "db/schema.sql";

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
				qDebug() << "SQLDatabase::getDb: pruned connection" << i.key() << "because it is no longer valid";
				i.remove();
			}
			//else qDebug() << "SQLDatabase::getDb: the connection" << i.key() << "is still intact";
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
					db->open(dbd.getDbname(), dbd.getHost(), dbd.getUser(), dbd.getPassword(), dbd.getPort());
				} break;

				case SQLConnectionDescription::SQLITE: {
					db = QSharedPointer<SQLDatabase>(new SQLiteDatabase(parent));
					db->open(file);
				} break;

				default: {
					qDebug() << "SQLDatabase::getDb: database type unknown, returning unopened database";
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
		db = QSharedPointer<SQLDatabase>(new SQLNullDatabase(parent));
	}

	return db;
}

/*
SQLDatabase *SQLDatabase::getDb(const QString& file, QObject *parent) {
	// check if the database wasn't open already
	// for now we'll return NULL, but we could build a map with all currently active connections
	if (QSqlDatabase::database(file).isValid()) {
		qDebug() << "SQLDatabase::openDb: connection with database referenced by '" << file << "' already exists";

		return NULL;
	}

	QFile f(file);

	QFileInfo fi(f);
	QString extension = fi.suffix();

	// if the file doesn't exist we return a special NULL database
	// but if the extension is supposed to be ".db" we can just create the database by opening with SQLite, so don't return yet
	if (!f.exists() && extension != "db") return new SQLNullDatabase(parent);

	SQLDatabase *db = NULL;

	SQLConnectionDescription dbd;

	if (extension == "db") {

	}
	else if (extension == "dbd" || extension == "xml") {

	}
	else {

	}

	if (extension == "db") {
		// assuming ".db" extension means SQLite database
		qDebug() << "SQLDatabase::openDb: opening new SQLite db, file =" << file;

		db = new SQLiteDatabase(parent);

		if (!db->open(file)) {
			qDebug() << "SQLDatabase::getDb: the open() call did not succeed, will return unopened SQLiteDatabase";
		}
	}
	else if (extension == "dbd" || extension == "xml") {
		// load data from the XML file
		// open database of the right type with this data

		SQLConnectionDescription dbd = SQLConnectionDescription(file);

		if (dbd.isValid()) {
			switch (dbd.getType()) {
				case SQLConnectionDescription::MYSQL: {
					db = new SQLMySqlDatabase(parent);
					db->open(dbd.getDbname(), dbd.getHost(), dbd.getUser(), dbd.getPassword(), dbd.getPort());
				} break;

				default: {
					qDebug() << "SQLDatabase::getDb: database type unknown, returning unopened database";
					db = new SQLNullDatabase(parent);
				} break;
			}
		}
		else {
			db = new SQLNullDatabase(parent);
		}

	}
	else {
		qDebug() << "SQLDatabase::openDb: unrecognized type of file to load";
		db = new SQLNullDatabase(parent);
	}

	return db;
}
*/

void SQLDatabase::saveConnectionInfo(const QString& file) const {
	if (!isOpen()) return;
	// will only write a file if isOpen() returns true, if it's a SQLite database it will make a copy of the database to this location
}

bool SQLDatabase::open(const QString& dbname, const QString& host, const QString& user, const QString& pass, int port) {
	return open(host + ":" + QString::number(port) + "/" + dbname, dbname, false, host, user, pass, port);
}

bool SQLDatabase::open(const QString& dbname) {
	return open(dbname, dbname, true);
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

	//mConnectionName = (dbnameOnly) ? "TheraSQL" : host + port + dbname;
	//mConnectionName = (dbnameOnly) ? dbname : host + port + dbname;
	mConnectionName = connName;

	qDebug() << "SQLDatabase::open: Trying to open database with connection name" << mConnectionName << "and driver" << mType;

	QSqlDatabase db = QSqlDatabase::addDatabase(mType, mConnectionName);

	if (db.isValid()) {
		if (!hasCorrectCapabilities()) {
			qDebug() << "SQLDatabase::open:" << mType << "Did not have all the correct capabilities, certain methods may fail";
		}

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

		if (db.open()) {
			setPragmas();

			if (!tables().contains("matches")) {
				qDebug() << "SQLDatabase::open: database opened correctly but was found to be empty, setting up Thera schema";

				setup(SCHEMA_FILE);
			}
			else {
				qDebug() << "SQLDatabase::open: database opened correctly and already contained tables:\n\t" << db.tables() << "\n\t" << tables();

				emit matchFieldsChanged();
			}

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

QString SQLDatabase::connectionName() const {
	return database().connectionName();
}

SQLDatabase::SQLDatabase(QObject *parent, const QString& type)
	: QObject(parent), mType(type) {
	QObject::connect(this, SIGNAL(databaseClosed()), this, SLOT(resetQueries()));
	QObject::connect(this, SIGNAL(matchFieldsChanged()), this, SLOT(makeFieldsSet()));
}

SQLDatabase::~SQLDatabase() {
	// copying and assigning are allowed now, we'd have to reference count OR rely on some master
	// object calling close() for us
	// TODO: re-evaluate this decision

	qDebug() << "SQLDatabase::~SQLDatabase: running, database is currently still" << (isOpen() ? "open" : "closed");

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

	if (!transaction()) {
		qDebug() << "SQLDatabase::addMatchField: could NOT start a transaction, the following might be very slow";
	}

	if (query.exec("START TRANSACTION")) qDebug() << "SQLDatabase::addMatchField: Correctly manually started a transaction";
	else qDebug() << "SQLDatabase::addMatchField can't start transaction:" << query.lastError();

	if (query.exec("BEGIN")) qDebug() << "SQLDatabase::addMatchField: Correctly manually started a transaction";
	else qDebug() << "SQLDatabase::addMatchField can't start transaction:" << query.lastError();

	success = query.exec(QString("CREATE TABLE %1 (match_id INTEGER PRIMARY KEY, %1 %2, confidence REAL)").arg(name).arg(sqlType));
	if (success) {
		// insert the default value everywhere
		query.prepare(QString(
			"INSERT INTO %1 (match_id, %1, confidence) "
			"VALUES (:match_id, :value, :confidence)"
		).arg(name));

		int i = 0;
		int step = 100;
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

				if (++i % step == 0) {
					qDebug() << "Inserted another" << step << "rows for field" << name << "now at" << i << "used" << timer.restart() << "msec";
				}
			}
		}
		else {
			qDebug() << "SQLDatabase::addMatchField couldn't create default values:" << idQuery.lastError()
				<< "\nQuery executed:" << idQuery.lastQuery();
		}

		qDebug() << "SQLDatabase::addMatchField succesfully created field:" << name;

		emit matchFieldsChanged();
	}
	else {
		qDebug() << "SQLDatabase::addMatchField couldn't create table:" << query.lastError()
			<< "\nQuery executed:" << query.lastQuery();
	}

	/*
	if (query.exec("ROLLBACK")) qDebug() << "SQLDatabase::addMatchField: Rolled back!";
	else qDebug() << "SQLDatabase::addMatchField: can't rollback" << query.lastError();
	*/

	/*
	{
		QSqlQuery commit(db);
		if (commit.exec("COMMIT;")) {
			qDebug() << "SQLDatabase::addMatchField: Correctly commited";
		}
		else {
			qDebug() << "SQLDatabase::addMatchField: couldn't commit" << commit.lastError();
		}
	}
	*/

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

	int fragments[IFragmentConf::MAX_FRAGMENTS];
	XF xf;

	QSqlQuery query(database());
	query.setForwardOnly(true);
	if (query.exec(queryString)) {
		while (query.next()) {
			QTextStream ts(query.value(3).toString().toAscii());
			ts >> xf;

			fragments[IFragmentConf::SOURCE] = Database::entryIndex(query.value(1).toString());
			fragments[IFragmentConf::TARGET] = Database::entryIndex(query.value(2).toString());

			list << SQLFragmentConf(this, query.value(0).toInt(), fragments, 1.0f, xf);
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

	QSqlDatabase db(database());

	transaction();

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

		emit databaseOpStepDone(i);

		++i;
	}

	commit();

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

	transaction();
	foreach (const QString &q, queries) {
		query.exec(q);
		qDebug() << "Executed query:" << q;
	}
	commit();

	emit matchFieldsChanged();
}

void SQLDatabase::close() {
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

	QSqlDatabase db(database());

	foreach (const QString& table, tables()) {
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
	foreach (const QString& table, tables(QSql::Views)) {
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
