#include "MatchModel.h"

#include <QDebug>
#include <QElapsedTimer>

#include "SQLFilter.h"
#include "MatchConflictChecker.h"

using namespace thera;

//MatchModel::MatchModel(SQLDatabase *db) : mDb(db), mFilter(db), mRealSize(0), mWindowSize(20), mWindowBegin(0), mWindowEnd(0) {
MatchModel::MatchModel(SQLDatabase *db, QObject *parent)
	: IMatchModel(parent),
	  mDb(NULL),
	  mPar(db),
	  mRealSize(0),
	  mWindowSize(20),
	  mNextWindowOffset(0),
	  mWindowOffset(0),
	  mWindowBegin(0),
	  mWindowEnd(0),
	  mDelayed(false),
	  mDirty(false),
	  mPreload(false) {
	setDatabase(db);
}

MatchModel::~MatchModel() {

}

void MatchModel::setDatabase(SQLDatabase *db) {
	if (mDb != db) {
		if (mDb) disconnect(mDb, 0, this, 0);

		mPar = ModelParameters(db);
		mDb = db;

		if (mDb) {
			connect(mDb, SIGNAL(matchCountChanged()), this, SLOT(databaseModified()));
			connect(mDb, SIGNAL(databaseClosed()), this, SLOT(databaseModified()));

			if (!mDb->isOpen()) {
				qDebug() << "MatchModel::setDatabase: Database was succesfully set, but it is still unopened";
			}
			else {
				// can't hurt
				mDb->addMetaMatchField("num_duplicates", "SELECT duplicate AS match_id, COUNT(duplicate) AS num_duplicates FROM duplicate GROUP BY duplicate");
			}
		}
		else {
			qDebug() << "MatchModel::setDatabase: passed database was NULL";
		}

		databaseModified();
	}
}

void MatchModel::prefetchHint(int start, int end) {
	assert(end > start);

	setWindowSize(end - start + 1);
	mNextWindowOffset = start % mWindowSize;

	//qDebug() << "Asked for prefetch of" << start << "to" << end << "(windowsize = " << mWindowSize << "and offset =" << mWindowOffset << ")";
}

void MatchModel::preloadMatchData(bool preload, const QStringList& fields) {
	mPreload = preload;

	if (preload && !fields.isEmpty()) {
		mPreloadFields = fields;
	}

	if (preload && fields.isEmpty() && mPreloadFields.isEmpty()) {
		mPreload = false;
	}
}

void MatchModel::setWindowSize(int size) {
	assert(size > 0);

	mWindowSize = size;
}

int MatchModel::getWindowSize() const {
	return mWindowSize;
}

bool MatchModel::requestWindow(int windowIndex) {
	if (windowIndex < 0) {
		mWindowOffset = 0;
		windowIndex = 0;
	}

	//qDebug("(1) Requested new window [old: %d, %d] [new: %d, %d], size: %d | offset = %d", mWindowBegin, mWindowEnd, windowIndex * mWindowSize + mWindowOffset, mWindowEnd, mWindowSize, mWindowOffset);

	mWindowBegin = windowIndex * mWindowSize + mNextWindowOffset;
	mWindowOffset = mNextWindowOffset;

	// gets overriden in populateModel, why bother?
	//mWindowEnd = (windowIndex + 1) * mWindowSize;

	/*
	if (!populateModel()) {
		// this really shouldn't happen, but it can if the database connection was unexpectedly closed for example
		requestRealSize();

		emit modelChanged();
	}
	*/

	return populateModel();
}

bool MatchModel::isValidIndex(int index) const {
	return (index >= 0) && (index < size());
}

int MatchModel::size() const {
	return mRealSize;
}

void MatchModel::sort(const QString& field, Qt::SortOrder order) {
	if (!mDb) return;

	if (mDelayed) {
		mDirty |= setSort(field, order, mDelayedPar);
	}
	else if (setSort(field, order, mPar)) {
		resetWindow();

		// don't populate the model yet, we don't know where the users of the model will look
		// and thus we could be fetching a window for naught
		//populateModel();

		emit orderChanged();
	}
}

void MatchModel::filter(const QString& pattern) {
	if (!mDb) return;

	qDebug() << "MatchModel::filter: in model, pattern:" << pattern;

	if (mDelayed) {
		mDirty |= setNameFilter(pattern, mDelayedPar);
	}
	else if (setNameFilter(pattern, mPar)) {
		qDebug() << "MatchModel::filter: Apparently change...";

		resetWindow();
		requestRealSize();

		// don't populate the model yet, we don't know where the users of the model will look
		// and thus we could be fetching a window for naught
		//populateModel();

		emit modelChanged();
	}
}

void MatchModel::genericFilter(const QString& key, const QString& filter) {
	if (!mDb) {
		qDebug() << "MatchModel::genericFilter: attempted to place generic filter on a model with an empty database";

		return;
	}

	if (mDelayed) {
		mDirty |= setGenericFilter(key, filter, mDelayedPar);
	}
	else if (setGenericFilter(key, filter, mPar)) {
		resetWindow();
		requestRealSize();

		// don't populate the model yet, we don't know where the users of the model will look
		// and thus we could be fetching a window for naught
		//populateModel();

		emit modelChanged();
	}
}

void MatchModel::neighbours(int index, NeighbourMode mode, bool keepParameters) {
	if (!mDb) return;

	if (!isValidIndex(index)) {
		qDebug() << "MatchModel::neighbours: index wasn't valid";

		return;
	}

	SQLFragmentConf *c = getSQL(index);

	if (c) neighbours(*c, mode, keepParameters);
}

inline void MatchModel::neighbours(const thera::SQLFragmentConf& match, NeighbourMode mode, bool keepParameters) {
	if (!keepParameters) {
		mPar.filter.clear();
		mPar.matchNameFilter.clear();
		mPar.sortField.clear();
	}

	mPar.neighbourMatchId = match.index();
	mPar.neighbourMode = mode;

	const QString allNeighboursFilter = QString("(target_name = '%1' OR source_name = '%2') OR (target_name = '%2' OR source_name = '%1')").arg(match.getSourceId()).arg(match.getTargetId());

	switch (mode) {
		case IMatchModel::ALL: {
			genericFilter("filter", allNeighboursFilter);

			qDebug() << "MatchModel::neighbours: got" << mRealSize << "matches to check for conflicts";
		} break;

		case IMatchModel::CONFLICTING: {
			SQLFilter filter(mDb);
			filter.setFilter("filter", allNeighboursFilter);

			QList<SQLFragmentConf> list = mDb->getMatches("error", Qt::AscendingOrder, filter);

			MatchConflictChecker checker(match, list);

			mMatches = checker.getConflicting();
			mRealSize = mMatches.size();
			mWindowBegin = 0;
			mWindowEnd = mRealSize - 1;

			emit modelChanged();
		} break;

		case IMatchModel::NONCONFLICTING: {
			SQLFilter filter(mDb);
			filter.setFilter("filter", allNeighboursFilter);

			QList<SQLFragmentConf> list = mDb->getMatches("error", Qt::AscendingOrder, filter);

			MatchConflictChecker checker(match, list);

			//mMatches = checker.getNonconflicting();
			mMatches = checker.getProgressiveNonconflicting();
			mRealSize = mMatches.size();
			mWindowBegin = 0;
			mWindowEnd = mRealSize - 1;

			emit modelChanged();
		} break;

		default:
			qDebug() << "MatchModel::neighbours: Unknown neighbourmode";
	}
}

void MatchModel::initBatchModification() {
	if (!mDb) return;

	if (!mDelayed) {
		mDelayedPar = mPar;

		mDelayed = true;
		mDirty = false;
	}
}

void MatchModel::endBatchModification() {
	if (!mDb) return;

	if (mDelayed) {
		mPar = mDelayedPar;

		mDelayed = false;

		if (mDirty) {
			mDirty = false;

			resetWindow();
			requestRealSize();

			// don't populate the model yet, we don't know where the users of the model will look
			// and thus we could be fetching a window for naught
			//populateModel();

			emit modelChanged();
		}
	}
}

IFragmentConf& MatchModel::get(int index) {
	//qDebug() << "get:" << index;

	SQLFragmentConf *conf = getSQL(index);

	return (conf) ? static_cast<IFragmentConf&>(*conf) : static_cast<IFragmentConf&>(mInvalidFragmentConf);
}

/*
inline thera::SQLFragmentConf& MatchModel::getSQL(int index) {
	//qDebug() << "Attempted pass:" << index << "<" << mWindowBegin << "||" << index << ">" << mWindowEnd << "| mMatches.size() =" << mMatches.size() << "and window size =" << mWindowSize;

	if (index < mWindowBegin || index > mWindowEnd) {
		// if the index is outside of the window, request another window in which it fits
		requestWindow((index - mNextWindowOffset) / mWindowSize);
	}

	//qDebug("MatchModel::getSQL: (%d - %d) %% %d = %d, ", index, mWindowOffset, mMatches.size(), (index - mWindowOffset) % mMatches.size());

	return mMatches[(index - mWindowOffset) % mMatches.size()];
}
*/

inline SQLFragmentConf *MatchModel::getSQL(int index) {
	if (index < mWindowBegin || index > mWindowEnd) {
		// if the index is outside of the window, request another window in which it fits
		if (!requestWindow((index - mNextWindowOffset) / mWindowSize)) return NULL;
	}

	if (mMatches.isEmpty()) return NULL;

	return &mMatches[(index - mWindowOffset) % mMatches.size()];
}

void MatchModel::setParameters(const ModelParameters& parameters) {
	if (parameters != mPar && parameters.db() == mDb) {
		//qDebug() << "MatchModel::setParameters:\n\t" << mPar.toString() << "!=\n\t" << parameters.toString();

		mPar = parameters;

		if (!mDb) return;

		if (mPar.neighbourMatchId != -1) {
			//qDebug() << "MatchModel::setParameters: Super special neighbourMatch mode!" << mPar.neighbourMatchId;

			neighbours(mDb->getMatch(mPar.neighbourMatchId), mPar.neighbourMode, true);
		}
		else {
			resetWindow();
			requestRealSize();

			emit modelChanged();
		}
	}
}

const ModelParameters& MatchModel::getParameters() const {
	return mPar;
}

bool MatchModel::addField(const QString& name, double defaultValue) {
	return (mDb) ? mDb->addMatchField(name, defaultValue) : false;
}

bool MatchModel::addField(const QString& name, const QString& defaultValue) {
	return (mDb) ? mDb->addMatchField(name, defaultValue) : false;
}

bool MatchModel::addField(const QString& name, int defaultValue) {
	return (mDb) ? mDb->addMatchField(name, defaultValue) : false;
}

bool MatchModel::removeField(const QString& name) {
	return (mDb) ? mDb->removeMatchField(name) : false;
}

QSet<QString> MatchModel::fieldList() const {
	return (mDb) ? mDb->matchFields() : QSet<QString>();
}

QString MatchModel::getFilter() const {
	return mPar.matchNameFilter;
}

/**
 * What do the duplicate modes do?
 *
 * ABSORB: all items in duplicates will have their entire respective groups (if they had one) pointing to the new master.
 * ORPHAN: all items that refered to an item that is in duplicates will now be their own master (i.e.: they will be orphaned)
 */
bool MatchModel::setDuplicates(QList<int> duplicatelist, int master, DuplicateMode mode) {
	// will create the num_duplicates view if it doesn't exist yet
	// TODO: if we switch to a remote database, we need to save on every query, so try to eliminate this one as it will in general be redundant
	//mDb->addMetaMatchField("num_duplicates", "SELECT duplicate AS match_id, COUNT(duplicate) AS num_duplicates FROM duplicate GROUP BY duplicate");

	if (!isValidIndex(master)) {
		qDebug() << "MatchModel::setDuplicates: master wasn't valid";

		return false;
	}

	QSet<int> duplicates = duplicatelist.toSet();
	duplicates.remove(master);

	// if the master to be was in a duplicate group, point all duplicates to it now
	// the master will almost certainly be in the current window so it should be
	// fast to retrieve it.
	IFragmentConf& conf = get(master);

	int newIdForStragglers = 0;
	switch (mode) {
		case ABSORB: newIdForStragglers = conf.index(); break;
		case ORPHAN: newIdForStragglers = 0; break;
		default: qDebug() << "MatchModel::setDuplicates: unknown duplicate mode";
	}

	// point all the matches referenced in the duplicates argument to the master
	qDebug() << "MatchModel::setDuplicates: NEW GROUP setting duplicate =" << conf.index() << "on" << duplicates.size() << "matches" << duplicates;
	foreach (int modelId, duplicates) {
		if (!isValidIndex(modelId)) { qDebug() << "MatchModel::setDuplicates: model id wasn't valid"; continue; }

		IFragmentConf& duplicate = get(modelId);
		int duplicateGroup = duplicate.getInt("duplicate", 0);

		if (duplicateGroup == 0) {
			// this means the duplicate was the master of a group, in which case we'll have to convert its group as well
			convertGroupToMaster(duplicate.index(), newIdForStragglers);
		}
		else {
			// this means the duplicate was already part of a group, but wasn't the master of it
			// we leave this old group alone because it is still intact
		}

		// this is likely unnecessary after convertGroupToMaster with ABSORB, but not harmful
		duplicate.setMetaData("duplicate", conf.index());
	}

	conf.setMetaData("duplicate", 0);

	return true;
}

bool MatchModel::setMaster(int master) {
	if (!isValidIndex(master)) {
		qDebug() << "MatchModel::setDuplicates: master wasn't valid";

		return false;
	}

	// if the master to be was in a duplicate group, point all duplicates to it now
	// the master will almost certainly be in the current window so it should be
	// fast to retrieve it.
	IFragmentConf& conf = get(master);

	int duplicateGroup = conf.getInt("duplicate", 0);

	// if the new master is already part of a group
	if (duplicateGroup != 0) {
		// set the parent/duplicate of the master to 0 (which means that it is the master of a group)
		conf.setMetaData("duplicate", 0);

		convertGroupToMaster(duplicateGroup, conf.index());
	}

	return true;
}

void MatchModel::convertGroupToMaster(int groupMatchId, int masterMatchId) {
	// first part of filter = all matches who have the same duplicate as the new master
	// second part of filter = the current master
	SQLFilter filter(mDb);
	filter.setFilter("filter", QString("duplicate = %1 OR matches.match_id = %1").arg(groupMatchId));

	QList<SQLFragmentConf> list = mDb->getMatches(QString(), Qt::AscendingOrder, filter);

	foreach (const SQLFragmentConf& c, list) {
		if (c.index() == masterMatchId) {
			c.setMetaData("duplicate", 0);
		}
		else {
			c.setMetaData("duplicate", masterMatchId);
		}
	}
}

bool MatchModel::populateModel() {
	QElapsedTimer timer;
	timer.start();

	// QStringList() << "status" << "volume" << "error" << "comment" << "num_duplicates"
	// QStringList() << "status" << "volume" << "error" << "comment" << "num_duplicates"
	if (mPreload) {
		mMatches = mDb->getPreloadedMatches(mPreloadFields, mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize);
	}
	else {
		mMatches = mDb->getMatches(mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize);
	}

	//mMatches = mDb->getMatches(mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize);
	mWindowEnd = mWindowBegin + mWindowSize - 1;

	if (mMatches.isEmpty()) {
		if (mDb->detectClosedDb()) {
			// emergency!
			qDebug() << "MatchModel::populateModel: database was apparently closed by an external factor";

			QTimer::singleShot(0, mDb, SLOT(close()));

			return false;
		}
		else if (mRealSize != 0) {
			// let's hope not receiving anything was the intention, still going to print out to easily detect unwanted errors
			// it shouldn't be frequent
			qDebug() << "MatchModel::populateModel: database still opened but getMatches() returned 0 matches and the real amount of matches was"
				"not 0, was this intentional? This could lead to segmentation faults";

			QTimer::singleShot(0, mDb, SLOT(close()));

			// for now we return false on this
			return false;
		}
	}

	// without a window
	//mMatches = mDb->getMatches(mSortField, mSortOrder, mFilter);
	//mRealSize = mMatches.size();

	qDebug() << "MatchModel::populateModel: Done repopulating model," << timer.elapsed() << "milliseconds";

	return true;
}

void MatchModel::requestRealSize() {
	QElapsedTimer timer;
	timer.start();

	mRealSize = mDb->getNumberOfMatches(mPar.filter);

	qDebug() << "MatchModel::requestRealSize: Get # of matches," << timer.elapsed() << "milliseconds [result" << mRealSize << "matches]";
}

void MatchModel::resetWindow() {
	mWindowBegin = 0;
	mWindowEnd = -1;
	//mWindowEnd = mWindowBegin + mWindowSize - 1;
}

/**
 * resets without firing signals
 */
void MatchModel::resetSort() {
	mPar.sortField = QString();
	mPar.sortOrder = Qt::AscendingOrder;
}

/**
 * resets without firing signals
 */
void MatchModel::resetFilter() {
	mPar.matchNameFilter = QString();
	mPar.filter.clear();
}

bool MatchModel::setSort(const QString& field, Qt::SortOrder order, ModelParameters& p) {
	if (!mDb->matchHasField(field)) {
		qDebug() << "MatchModel::setSort: Field" << field << "doesn't exist";
	}
	else if (p.sortField != field || p.sortOrder != order) {
		p.sortField = field;
		p.sortOrder = order;

		return true;
	}

	return false;
}

bool MatchModel::setNameFilter(const QString& pattern, ModelParameters& p) {
	if (p.matchNameFilter != pattern) {
		if (!pattern.isEmpty()) {
			QString normalizedFilter = pattern;

			/*
			// escape '_' and '%' (we possibly might have to move this to makeCompatible but for now it looks like all databases support it
			QString newStatement = filter;
			QRegExp rx("\\bLIKE\\s+'(\\S*)'(?!\\s+ESCAPE\\b)"); // advanced version that doesn't try to escape already escaped clauses
			int pos = 0;

			while ((pos = rx.indexIn(filter, pos)) != -1) {
				QString c;

				if (!(c = rx.cap(1)).isEmpty()) {
					// LIKE 'bloee_bla' ==> LIKE 'bloee\_bla' ESCAPE '\'
					// c == 'bloee_bla'
					QString escapedLikeTerm = c.replace("_", "\\_").replace("%", "\\%");
					qDebug() << "c = " << c << "\n\tescapedLikeTerm =" << escapedLikeTerm;
					newStatement = newStatement.replace(rx.cap(0), "LIKE '" + escapedLikeTerm + "' ESCAPE '\\'");
				}

				pos += rx.matchedLength();
			}

			qDebug() << "FILTER =" << filter << "\n\tVS NEWSTATEMENT =" << newStatement;
			*/

			if (!normalizedFilter.startsWith('*')) normalizedFilter.prepend("*");
			if (!normalizedFilter.endsWith('*')) normalizedFilter.append("*");

			QString escape = mDb->escapeCharacter();
			bool hasDefaultEscape = !escape.isEmpty();

			if (!hasDefaultEscape) {
				escape = "°";
			}

			normalizedFilter = normalizedFilter.replace("_", escape + "_").replace("%", escape + "%");
			normalizedFilter = normalizedFilter.replace("*","%").replace("?","_");

			if (!hasDefaultEscape) {
				p.filter.setFilter("matchmodel_names", QString("source_name || target_name LIKE '%1' ESCAPE '%2' OR target_name || source_name LIKE '%1' ESCAPE '%2'").arg(normalizedFilter).arg(escape));
			}
			else {
				p.filter.setFilter("matchmodel_names", QString("source_name || target_name LIKE '%1' OR target_name || source_name LIKE '%1'").arg(normalizedFilter));
			}
		}
		else {
			p.filter.removeFilter("matchmodel_names");
		}

		p.matchNameFilter = pattern;

		return true;
	}

	return false;
}

bool MatchModel::setGenericFilter(const QString& key, const QString& filter, ModelParameters& p) {
	if (!p.filter.hasFilter(key, filter)) {
		if (!filter.isEmpty()) p.filter.setFilter(key, filter);
		else p.filter.removeFilter(key);

		return true;
	}

	return false;
}

void MatchModel::databaseModified() {
	resetSort();
	resetFilter();
	resetWindow();

	if (mDb && mDb->isOpen()) {
		requestRealSize();
		populateModel();

		emit modelChanged();
	}
	else if (mRealSize != 0 || mMatches.size() != 0) {
		if (mDb) qDebug() << "MatchModel::databaseModified: closed database" << mDb->connectionName() << "but real size or matches size is not 0" << mRealSize << mMatches.size();
		else qDebug() << "MatchModel::databaseModified: NULL database but real size or matches size is not 0" << mRealSize << mMatches.size();

		mRealSize = 0;
		mMatches.clear();

		emit modelChanged();
	}

	QString dbName = (mDb) ? mDb->connectionName() : "NULL pointer";

	qDebug() << "MatchModel::databaseModified: the database" << dbName << "was modified, now available:" << size();
}
