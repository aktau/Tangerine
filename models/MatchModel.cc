#include "MatchModel.h"

#include <QDebug>
#include <QElapsedTimer>

#include "SQLFilter.h"
#include "MatchConflictChecker.h"

#ifdef IS_TANGERINE
#include "main.h"
#endif

using namespace thera;

//MatchModel::MatchModel(SQLDatabase *db) : mDb(db), mFilter(db), mRealSize(0), mWindowSize(20), mWindowBegin(0), mWindowEnd(0) {
MatchModel::MatchModel(SQLDatabase *db, int refreshInterval, QObject *parent)
	: IMatchModel(parent),
	  mDb(NULL),
	  mPar(db),
	  mLoadedWindowBegin(0),
	  mRealSize(0),
	  mWindowSize(20),
	  mNextWindowOffset(0),
	  mWindowOffset(0),
	  mWindowBegin(0),
	  mWindowEnd(0),
	  mDelayed(false),
	  mDirty(false),
	  mPreload(false),
	  mRefreshTimer(new QTimer(this)),
	  mBaseRefreshInterval(refreshInterval) {
	setDatabase(db);

	if (refreshInterval > 0) {
		//QTimer *timer = new QTimer(this);
		connect(mRefreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
		mRefreshTimer->start(refreshInterval);

#ifdef IS_TANGERINE
		TangerineApplication *app = qobject_cast<TangerineApplication *>(QApplication::instance());

		if (app) {
			connect(app, SIGNAL(activated()), mRefreshTimer, SLOT(start()));
			connect(app, SIGNAL(deactivated()), mRefreshTimer, SLOT(stop()));
		}
#endif

		// restart the timer whenever we just checked manually
		connect(this, SIGNAL(modelChanged()), mRefreshTimer, SLOT(start()));
		connect(this, SIGNAL(orderChanged()), mRefreshTimer, SLOT(start()));
	}
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

			SQLQueryParameters parameters = SQLQueryParameters((mPreload) ? mPreloadFields : QStringList(), mPar.sortField, mPar.sortOrder, filter);
			QList<SQLFragmentConf> list = mDb->getMatches(parameters);
			//QList<SQLFragmentConf> list = mDb->getMatches("error", Qt::AscendingOrder, filter);

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

			SQLQueryParameters parameters = SQLQueryParameters((mPreload) ? mPreloadFields : QStringList(), mPar.sortField, mPar.sortOrder, filter);
			QList<SQLFragmentConf> list = mDb->getMatches(parameters);
			//QList<SQLFragmentConf> list = mDb->getMatches("error", Qt::AscendingOrder, filter);

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

inline SQLFragmentConf *MatchModel::getSQL(int index) {
	if (index < mWindowBegin || index > mWindowEnd) {
		//qDebug() << "it seems we're outta window:" << mWindowBegin << "<" << index << "<" << mWindowEnd;

		// if the index is outside of the window, request another window in which it fits
		if (!requestWindow((index - mNextWindowOffset) / mWindowSize)) return NULL;
	}

	if (mMatches.isEmpty()) return NULL;

	// in case we fetched a succesful but incomplete window AND the user is trying to fetch inside the incomplete part
	if (mMatches.size() < mWindowSize && (((index - mWindowOffset) % mWindowSize) >= mMatches.size())) {
		qDebug() << "MatchModel::getSQL: fetched unfilled part of incomplete window, returning invalid fragment" << index << "window has:" << mMatches.size() << "/" << mWindowSize;

		return NULL;
	}

	// in general, mMatches.size() == mWindowSize, but in case it somehow gets desynced, it's safer (non-segmentation-faultier)
	// to modulo this way
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

	foreach (const SQLFragmentConf& c, mMatches) {
		c.clearCache("duplicate");
		c.clearCache("num_duplicates");
	}

	refresh(true);

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

		// num_duplicates will be stale after this
		// someday, all of this will be automated, it will be a better world
		// an idea for a start:
		// all caches are managed by the SQLDatabase, the SQLFragmentConf request them when necessary like this:
		// 		mDb->getCache(...)
		// the db could scan all queries and invalidate caches on an automatic basis
		//
		// almost scratch that, even better: mDb->getMetaData(...), which already exists, could just fetch from a cache, right?
		// But... how will the database know when to delete a cache? It can't keep smart pointers to all of its SQLFragmentConf's
		// ---> true, but the caching system is supposed to be transparant, so it's not too bad if the db starts dropping caches as if they were smoldering
		// 		when it has reached a certain limit. You know, LRU and stuff. Man... it sure feels like we're repeating stuff over here!
		foreach (const SQLFragmentConf& c, mMatches) {
			c.clearCache("duplicate");
			c.clearCache("num_duplicates");
		}

		// in fact, for now it's just cheaper to just refresh EVERYTHING in one go, because clearing the cache for num_duplicates will cause 20 small queries
		// 1 big one usually performs better, unless it's REALLY big (we hope not)
		// TODO: we could make a refreh that doesn't emit a signal, and rely on the VIEW to reload itself when we return true
		refresh();
	}

	return true;
}

bool MatchModel::resetDuplicates(QList<int> duplicates) {
	// point all the matches referenced in the duplicates argument to the master
	qDebug() << "MatchModel::resetDuplicates: resetting" << duplicates.size() << "matches:" << duplicates;
	foreach (int modelId, duplicates) {
		if (!isValidIndex(modelId)) { qDebug() << "MatchModel::resetDuplicates: model id wasn't valid"; continue; }

		IFragmentConf& duplicate = get(modelId);
		int duplicateGroup = duplicate.getInt("duplicate", 0);

		duplicate.setMetaData("duplicate", 0);

		if (duplicateGroup != 0) {
			// this means the duplicate was the master of a group, in which case we'll have to convert its group as well
			convertGroupToMaster(duplicate.index(), 0);
		}
	}

	foreach (const SQLFragmentConf& c, mMatches) {
		c.clearCache("duplicate");
		c.clearCache("num_duplicates");
	}

	refresh(true);

	return true;
}

void MatchModel::convertGroupToMaster(int groupMatchId, int masterMatchId) {
	// first part of filter = all matches who have the same duplicate as the new master
	// second part of filter = the current master
	SQLFilter filter(mDb);
	filter.setFilter("filter", QString("duplicate = %1 OR matches.match_id = %1").arg(groupMatchId));

	SQLQueryParameters parameters = SQLQueryParameters(QStringList(), QString(), Qt::AscendingOrder, filter);
	QList<SQLFragmentConf> list = mDb->getMatches(parameters);
	//QList<SQLFragmentConf> list = mDb->getMatches(QString(), Qt::AscendingOrder, filter);

	foreach (const SQLFragmentConf& c, list) {
		if (c.index() == masterMatchId) {
			qDebug() << "MatchModel::convertGroupToMaster: making pair" << c.index() << "the master of group" << masterMatchId;
			c.setMetaData("duplicate", 0);
		}
		else {
			qDebug() << "MatchModel::convertGroupToMaster: making pair" << c.index() << "a duplicate of" << masterMatchId;
			c.setMetaData("duplicate", masterMatchId);
		}
	}
}

void MatchModel::refresh(bool forceReloadOnConflict) {
	if (mMatches.isEmpty() || mDelayed) return;

	QList<SQLFragmentConf> matches = fetchCurrentMatches();

	qDebug() << "MatchModel::refresh: refresh called with non-empty matches, let's see if anything needs updating..." << mMatches.size() << "and" << matches.size();

	if (matches.size() != mMatches.size()) {
		qDebug() << "MatchModel::refresh: can't refresh, the size of the resultset must have changed because of some external reason";

		// the size of the resultset is not the same
		// this is pretty serious business

		if (forceReloadOnConflict) {
			if (!matches.isEmpty()) {
				// apparently we're still in a valid window

				// should hopefully be cheap, as we just did that query with a LIMIT
				requestRealSize();

				// let's not reset the window, that would be superfluous as we already have the new window (we just fetched it)
				// if the user doesn't like it he'll ask for a new window himself after the modelChanged() signal
				//resetWindow();

				// cans and cans of worms, basically if we don't set mWindowEnd to what it _should_ have
				// been, then the model will try to request a new window and fail at it every time an index
				// outside of the decreased window is tried, so in the end it will make no difference except that
				// this way saves a lot of DB requests
				mMatches = matches;
				mWindowEnd = mWindowBegin + mWindowSize - 1;

				emit modelChanged();
			}
			else {
				// if it's empty, let's not do anything, could be dangerous (TODO: find an ACTUAL solution)
			}
		}

		return;
	}

	// test if any value differs from an already cached value, if it does, prepare to send a refresh
	QList<SQLFragmentConf>::const_iterator newConf = matches.constBegin();
	QList<SQLFragmentConf>::iterator oldConf = mMatches.begin();

	bool refreshNeeded = false;

	for (; newConf != matches.constEnd(); ++newConf, ++oldConf) {
		if (oldConf->index() == newConf->index()) {
			refreshNeeded |= !oldConf->absorb(*newConf);
		}
		else {
			if (!forceReloadOnConflict) {
				qDebug() << "MatchModel::refresh: the resultset of the query apparently changed because of external reasons, not refreshing";
			}
			else {
				// abort all this merging business and just replace it, stuff has gone haywire!

				// it is dubitable whether this is a good idea, it probably wouldn't hurt too much if
				// we didn't and we'd save another potentially horrible query
				requestRealSize();

				// let's not reset the window, that would be superfluous as we already have the new window (we just fetched it)
				// if the user doesn't like it he'll ask for a new window himself after the modelChanged() signal
				//resetWindow();

				// cans and cans of worms, basically if we don't set mWindowEnd to what it _should_ have
				// been, then the model will try to request a new window and fail at it every time an index
				// outside of the decreased window is tried, so in the end it will make no difference except that
				// this way saves a lot of DB requests
				mMatches = matches;
				mWindowEnd = mWindowBegin + mWindowSize - 1;

				emit modelChanged();
			}

			return;
		}
	}

	if (refreshNeeded) {
		qDebug() << "MatchModel::refresh: refresh was necessary and completed succesfully, sending modelRefreshed() signal";

		emit modelRefreshed();
	}
}

bool MatchModel::populateModel() {
	mMatches = fetchCurrentMatches();

	// we should probably really check if this is correct, because the amount of matches returned could be smaller
	// that said, I believe segmentation faults are no longer an issue because an InvalidFragmentConf will be returned instead
	mWindowEnd = mWindowBegin + mWindowSize - 1;
	//mWindowEnd = mWindowBegin + mMatches.size() - 1;

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

	qDebug() << "MatchModel::populateModel: Done repopulating model," << mLastQueryMsec << "milliseconds";

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
	mMatches.clear();
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
				escape = "�";
			}

			normalizedFilter = normalizedFilter.replace("_", escape + "_").replace("%", escape + "%");
			normalizedFilter = normalizedFilter.replace("*","%").replace("?","_");

			// all this is necessary because in MySQL, when you compare an escaped string with LIKE inside a VIEW, it's difficult not to
			// get collation errors. There appears to be no clean fix, I've tried many things (changing connection charset, etc). My dirty
			// solution is to not provide an explicit ESCAPE because it already provides the backslash.
			// MySQL wasted 3 hours of my precious time today...
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

// more likely inlining
inline const QList<thera::SQLFragmentConf> MatchModel::fetchCurrentMatches() {
	QElapsedTimer timer;
	timer.start();

	//int limit, int extremeMatchId, int extremeSortValue, bool forward, int offset

	int loadedWindowEnd = mLoadedWindowBegin + mMatches.size();
	int requestedWindowEnd = mWindowBegin + mWindowSize;

	QList<thera::SQLFragmentConf> list;
	SQLQueryParameters parameters = SQLQueryParameters((mPreload) ? mPreloadFields : QStringList(), mPar.sortField, mPar.sortOrder, mPar.filter);

	if (!mMatches.isEmpty() && mWindowSize > 0) {
		qDebug("MatchModel::fetchCurrentMatches: [PAGINATION] moving window [%d,%d] to window [%d,%d]", mLoadedWindowBegin, loadedWindowEnd, mWindowBegin, requestedWindowEnd);
		// find the reference fragment

		// detect special case where the old window is inside of the new window (and the new window is bigger on both sides)
		if (mWindowBegin < mLoadedWindowBegin && requestedWindowEnd > loadedWindowEnd) {
			qDebug("MatchModel::fetchCurrentMatches: [PAGINATION -> STANDARD] fail, older window strictly smaller [%d,%d] than new window [%d,%d], switching to standard query", mLoadedWindowBegin, loadedWindowEnd, mWindowBegin, requestedWindowEnd);

			parameters.moveToAbsoluteWindow(mWindowBegin, mWindowSize);

			//list = (mPreload) ?
			//	mDb->getPreloadedMatches(mPreloadFields, mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize) :
			//	mDb->getMatches(mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize);

			qDebug("MatchModel::fetchCurrentMatches: [STANDARD] done");
		}
		else {
			// in all the other cases, we can request the new window in one go
			if (mWindowBegin >= mLoadedWindowBegin) {
				// can be fetched completely by forward searching (looking ahead)
				bool inclusive = mWindowBegin == mLoadedWindowBegin;

				int referenceIndex = qMin(loadedWindowEnd, mWindowBegin) - 1 + inclusive;
				int offset = qMax(0, mWindowBegin - loadedWindowEnd);

				qDebug("MatchModel::fetchCurrentMatches: [PAGINATION-FORWARD]: referenceIndex = %d, offset = %d, inclusive = %d => (index - mWindowOffset) %% mMatches.size() = (%d - %d) %% %d)",
								referenceIndex, offset, inclusive, referenceIndex, mWindowOffset, mMatches.size());

				const thera::SQLFragmentConf &conf = mMatches.at((referenceIndex - mWindowOffset) % mMatches.size());

				int matchIdExtremeValue = conf.index();
				double sortFieldExtremeValue = conf.getDouble(mPar.sortField, 0.0);

				qDebug("MatchModel::fetchCurrentMatches: [PAGINATION-FORWARD]: extreme values for referenceIndex %d [match: %d, sort: %f]", referenceIndex, matchIdExtremeValue, sortFieldExtremeValue);

				/*
				list = mDb->getFastPaginatedPreloadedMatches(
					mPreloadFields,
					mPar.sortField,
					mPar.sortOrder,
					mPar.filter,
					mWindowSize,
					matchIdExtremeValue,
					sortFieldExtremeValue,
					true,
					inclusive,
					offset);
				*/

				//list = mDb->getMatches(parameters);
				parameters.moveToRelativeWindow(conf, inclusive, true, offset, mWindowSize);
			}
			else if (requestedWindowEnd <= loadedWindowEnd) {
				// can be fetched completely by backward searching
				bool inclusive = requestedWindowEnd == loadedWindowEnd;

				int referenceIndex = qMax(mLoadedWindowBegin, requestedWindowEnd) - inclusive;
				int offset = qMax(0, mLoadedWindowBegin - requestedWindowEnd);

				qDebug("MatchModel::fetchCurrentMatches: [PAGINATION-BACKWARD]: referenceIndex = %d, offset = %d, inclusive = %d => (index - mWindowOffset) %% mMatches.size() = (%d - %d) %% %d)",
					referenceIndex, offset, inclusive, referenceIndex, mWindowOffset, mMatches.size());

				const thera::SQLFragmentConf &conf = mMatches.at((referenceIndex - mWindowOffset) % mMatches.size());

				int matchIdExtremeValue = conf.index();
				double sortFieldExtremeValue = conf.getDouble(mPar.sortField, 0.0);

				qDebug("MatchModel::fetchCurrentMatches: [PAGINATION-BACKWARD]: extreme values for referenceIndex %d [match: %d, sort: %f]", referenceIndex, matchIdExtremeValue, sortFieldExtremeValue);

				/*
				list = mDb->getFastPaginatedPreloadedMatches(
					mPreloadFields,
					mPar.sortField,
					mPar.sortOrder,
					mPar.filter,
					mWindowSize,
					matchIdExtremeValue,
					sortFieldExtremeValue,
					false,
					inclusive,
					offset);
				*/

				parameters.moveToRelativeWindow(conf, inclusive, false, offset, mWindowSize);
			}
			else {
				qDebug("MatchModel::fetchCurrentMatches: [PAGINATION] shouldn't be here: [%d,%d] to window [%d,%d]", mLoadedWindowBegin, loadedWindowEnd, mWindowBegin, requestedWindowEnd);
			}

			qDebug("MatchModel::fetchCurrentMatches: [PAGINATION]: Done, got %d matches", list.size());
		}
	}
	else {
		qDebug("MatchModel::fetchCurrentMatches: [NO] doing it the standard way because [%d,%d] to window [%d,%d]", mLoadedWindowBegin, loadedWindowEnd, mWindowBegin, requestedWindowEnd);

		parameters.moveToAbsoluteWindow(mWindowBegin, mWindowSize);

		/*
		list = (mPreload) ?
			mDb->getPreloadedMatches(mPreloadFields, mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize) :
			mDb->getMatches(mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize);
		*/

		qDebug("MatchModel::fetchCurrentMatches: [NO] done");
	}

	list = mDb->getMatches(parameters);

	mLastQueryMsec = timer.elapsed();

	// save which window is currently active
	mLoadedWindowBegin = mWindowBegin;

	// it's possibly this method gets called not by refresh(), in which case we'll also want to restart the timer, it makes no
	// sense to refresh when we just fetched, also perhaps the interval needs some heuristic setting
	autoAdjustRefreshTimer();

	return list;
}

inline void MatchModel::autoAdjustRefreshTimer() {
	if (!mRefreshTimer) return;

	// start or restart
	mRefreshTimer->start();

	// if the database call took longer than 100 ms we're possibly having
	// 		slow internet
	// 		a slow query
	//		database server overloaded
	// --> the good thing to do is to throttle the amount of refreshes
	// TODO: provide an option to disable throttling
	if (mLastQueryMsec < 100) {
		if (mRefreshTimer->interval() > mBaseRefreshInterval) {
			// at some point in time the interval was set higher than the original interval because the db fetch was slow, it's fast again
			// so let's return to the default interval
			mRefreshTimer->setInterval(mBaseRefreshInterval);

			qDebug() << "MatchModel::autoAdjustRefreshTimer: last query took < 100 msec, putting refresh timer back to normal = " << mBaseRefreshInterval << "msec";
		}
	}
	else if (mLastQueryMsec < 500) {
		// the db fetch was pretty slow (more than 100 msec might be noticeable and possibly a drain on external DB resources)
		// but we're going to give it the benefit of the doubt and raise the refresh limit until it reaches once every 5 minutes
		const int newInterval = qMin(1000 * 60 * 5, mRefreshTimer->interval() * 2); // 5 minutes max

		if (newInterval != mRefreshTimer->interval()) {
			qDebug() << "MatchModel::autoAdjustRefreshTimer: (last query took" << mLastQueryMsec << "msec, raised the refresh interval from"
				<< (float(mRefreshTimer->interval()) / 1000.f) << "to" << (float(newInterval) / 1000.f)
				<< "seconds because of slow database fetching (conserve resources)";

			mRefreshTimer->setInterval(newInterval);
		}
	}
	else {
		// the query took more than half a second, that's too slow

		qDebug() << "MatchModel::autoAdjustRefreshTimer: (last query took" << mLastQueryMsec << "msec, stopped refreshing until a cheaper query is made";

		// put the interval to something ridiculously high because the fetching time was way too long
		mRefreshTimer->setInterval(100000000); // that's right, a 100 million seconds
		mRefreshTimer->stop(); // we can't stop it indefinitely because focusing the TangerineApplication could restart it, it doesn't matter with such an interval though
	}
}
