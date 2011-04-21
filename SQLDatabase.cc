#include "SQLDatabase.h"

#include <QtSql>
#include <QFile>
#include <QTextStream>

#include "XF.h"

using namespace thera;

const QString SQLDatabase::CONN_NAME = "TheraSQL";
const QString SQLDatabase::SCHEMA_FILE = "db/schema.sql";
const QString SQLDatabase::DB_HOST = "localhost";

const QString SQLDatabase::MATCHES_ROOTTAG = "matches";
const QString SQLDatabase::MATCHES_DOCTYPE = "matches-cache";
const QString SQLDatabase::OLD_MATCHES_VERSION = "0.0";
const QString SQLDatabase::MATCHES_VERSION = "1.0";

QStringList SQLDatabase::FIELDS = QStringList() << "status" <<  "error" << "overlap" << "volume" << "old_volume";

SQLDatabase::SQLDatabase() {
	/* nothing */
}

SQLDatabase::~SQLDatabase() {
	close();
}

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

void SQLDatabase::saveToXML(const QString& XMLFile) const {
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

QList<SQLFragmentConf> SQLDatabase::getAllMatches() {
	QList<SQLFragmentConf> list;

	QSqlQuery query(database());
	query.setForwardOnly(true);
	if (query.exec("SELECT matches.match_id, source_name, target_name, transformation FROM matches")) {
		while (query.next()) {
			SQLFragmentConf fc(this, query.value(0).toInt());

			//qDebug() << query.value(1).toString() << "->" << Database::entryIndex(query.value(1).toString()) << "||" << query.value(2).toString() << "->" << Database::entryIndex(query.value(2).toString());

			fc.mFragments[IFragmentConf::SOURCE] = Database::entryIndex(query.value(1).toString());
			fc.mFragments[IFragmentConf::TARGET] = Database::entryIndex(query.value(2).toString());
			fc.mRelev = 1.0f; // placeholder; we should compute relev based on err here

			XF xf;
			QTextStream ts(query.value(2).toString().toAscii());
			ts >> xf;

			list.append(fc);
		}
	}
	else {
		qDebug() << "Query failed:" << query.lastError();
	}

	return list;
}

/**
 * TODO: implement
 */
const QDomDocument SQLDatabase::toXML() const {
	if (!isOpen()) {
		qDebug() << "Database wasn't open, couldn't convert to XML";

		return QDomDocument();
	}

	QDomDocument doc(MATCHES_DOCTYPE);
	QDomElement matches = doc.createElement(MATCHES_ROOTTAG);
	matches.setAttribute("version", MATCHES_VERSION);

	doc.appendChild(matches);

	return doc;
}

int SQLDatabase::matchCount() const {
	if (!isOpen()) {
		return 0;
	}

	QSqlDatabase db(database());
	QSqlQuery query(db);

	if (query.exec("SELECT Count(*) FROM matches") && query.first()) {
		return query.value(0).toInt();
	}
	else {
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
		QString conflictString = match.attribute("conflict", "");
		QStringList conflicts = conflictString.split(" ");

		foreach (const QString &c, conflicts) {
			/*
			query.prepare(
				"INSERT INTO conflicts (match_id, other_match_id) "
				"VALUES (:match_id, :other_match_id)"
			);
			*/
			conflictsQuery.bindValue(":match_id", matchId);
			conflictsQuery.bindValue(":other_match_id", c.toInt());
			conflictsQuery.exec();
		}

		emit databaseOpStepDone(i);

		++i;
	}

	db.commit();

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
	QString schemaQuery = QString(data);
	QStringList queries = schemaQuery.split(";");
	QSqlQuery query(db);

	db.transaction();
	foreach (const QString &q, queries) {
		query.exec(q);
		qDebug() << "Executed query:" << q;
	}
	db.commit();

	file.close();
}

void SQLDatabase::close() {
	if (isOpen()) {
		database().close();

		emit databaseClosed();
	}
}

bool SQLDatabase::matchHasField(const QString& field) const {
	return FIELDS.contains(field, Qt::CaseInsensitive);
}
