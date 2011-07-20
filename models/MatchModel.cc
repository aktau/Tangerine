#include "MatchModel.h"

#include <QDebug>
#include <QElapsedTimer>

#include "SQLFilter.h"
#include "MatchConflictChecker.h"

using namespace thera;

//MatchModel::MatchModel(SQLDatabase *db) : mDb(db), mFilter(db), mRealSize(0), mWindowSize(20), mWindowBegin(0), mWindowEnd(0) {
MatchModel::MatchModel(SQLDatabase *db) : mDb(db), mPar(db), mRealSize(0), mWindowSize(20), mWindowOffset(0), mWindowBegin(0), mWindowEnd(0), mDelayed(false), mDirty(false) {
	if (mDb == NULL) {
		qDebug() << "MatchModel::MatchModel: passed database was NULL, this will lead to trouble";
	}
	else {
		//connect(mDb, SIGNAL(matchCountChanged()), this, SLOT(matchCountChanged()));
		connect(mDb, SIGNAL(matchCountChanged()), this, SLOT(databaseModified()));

		if (!mDb->isOpen()) {
			qDebug() << "MatchModel::MatchModel: Closed database was passed, this will lead to trouble...";
		}
		else {
			databaseModified();
		}
	}
}

MatchModel::~MatchModel() {

}

void MatchModel::prefetchHint(int start, int end) {
	assert(end > start);

	setWindowSize(end - start + 1);
	mWindowOffset = start % mWindowSize;

	//qDebug() << "Asked for prefetch of" << start << "to" << end << "(windowsize = " << mWindowSize << "and offset =" << mWindowOffset << ")";
}

void MatchModel::setWindowSize(int size) {
	assert(size > 0);

	mWindowSize = size;
}

int MatchModel::getWindowSize() const {
	return mWindowSize;
}

void MatchModel::requestWindow(int windowIndex) {
	if (windowIndex < 0) {
		mWindowOffset = 0;
		windowIndex = 0;
	}

	mWindowBegin = windowIndex * mWindowSize + mWindowOffset;

	// gets overriden in populateModel, why bother?
	//mWindowEnd = (windowIndex + 1) * mWindowSize;

	//qDebug() << "Requested new window" << mWindowBegin << "," << mWindowEnd << "," << mWindowSize;

	populateModel();
}

bool MatchModel::isValidIndex(int index) const {
	return (index >= 0) && (index < size());
}

int MatchModel::size() const {
	return mRealSize;
}

void MatchModel::sort(const QString& field, Qt::SortOrder order) {
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
	SQLFragmentConf& c = getSQL(index);

	if (!keepParameters) mPar.filter.clear();

	const QString allNeighboursFilter = QString("(target_name = '%1' OR source_name = '%2') OR (target_name = '%2' OR source_name = '%1')").arg(c.getSourceId()).arg(c.getTargetId());

	switch (mode) {
		case IMatchModel::ALL: {
			genericFilter("filter", allNeighboursFilter);

			qDebug() << "MatchModel::neighbours: got" << mRealSize << "matches to check for conflicts";
		} break;

		case IMatchModel::CONFLICTING: {
			SQLFilter filter(mDb);
			filter.setFilter("filter", allNeighboursFilter);

			QList<SQLFragmentConf> list = mDb->getMatches("error", Qt::AscendingOrder, filter);

			MatchConflictChecker checker(c, list);

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

			MatchConflictChecker checker(c, list);

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
	if (!mDelayed) {
		mDelayedPar = mPar;

		mDelayed = true;
		mDirty = false;
	}
}

void MatchModel::endBatchModification() {
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

thera::IFragmentConf& MatchModel::get(int index) {
	return getSQL(index);
}

inline thera::SQLFragmentConf& MatchModel::getSQL(int index) {
	//qDebug() << "Attempted pass:" << index << "<" << mWindowBegin << "||" << index << ">" << mWindowEnd << "| mMatches.size() =" << mMatches.size() << "and window size =" << mWindowSize;

	if (index < mWindowBegin || index > mWindowEnd) {
		// if the index is outside of the window, request another window in which it fits
		requestWindow((index - mWindowOffset) / mWindowSize);
	}

	//return mMatches[index % mWindowSize];
	return mMatches[index % mMatches.size()];
}

void MatchModel::setParameters(const ModelParameters& parameters) {
	if (parameters != mPar) {
		mPar = parameters;

		resetWindow();
		requestRealSize();

		emit modelChanged();
	}
}

const ModelParameters& MatchModel::getParameters() const {
	return mPar;
}

bool MatchModel::addField(const QString& name, double defaultValue) {
	return mDb->addMatchField(name, defaultValue);
}

bool MatchModel::addField(const QString& name, const QString& defaultValue) {
	return mDb->addMatchField(name, defaultValue);
}

bool MatchModel::addField(const QString& name, int defaultValue) {
	return mDb->addMatchField(name, defaultValue);
}

bool MatchModel::removeField(const QString& name) {
	return mDb->removeMatchField(name);
}

QSet<QString> MatchModel::fieldList() const {
	return mDb->matchFields();
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

void MatchModel::populateModel() {
	QElapsedTimer timer;
	timer.start();

	mMatches = mDb->getMatches(mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize);
	mWindowEnd = mWindowBegin + mWindowSize - 1;

	// without a window
	//mMatches = mDb->getMatches(mSortField, mSortOrder, mFilter);
	//mRealSize = mMatches.size();

	qDebug() << "MatchModel::populateModel: Done repopulating model," << timer.elapsed() << "milliseconds";
}

void MatchModel::requestRealSize() {
	QElapsedTimer timer;
	timer.start();

	mRealSize = mDb->getNumberOfMatches(mPar.filter);

	qDebug() << "MatchModel::populateModel: Get # of matches," << timer.elapsed() << "milliseconds [result" << mRealSize << "matches]";
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

			if (!normalizedFilter.startsWith('*')) normalizedFilter.prepend("*");
			if (!normalizedFilter.endsWith('*')) normalizedFilter.append("*");

			normalizedFilter = QString(normalizedFilter).replace("*","%").replace("?","_");

			p.filter.setFilter("matchmodel_names", QString("source_name || target_name LIKE '%1' OR target_name || source_name LIKE '%1'").arg(normalizedFilter));
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

	requestRealSize();
	populateModel();

	emit modelChanged();

	qDebug() << "MatchModel::databaseModified: apparently, the database was modified, now available:" << size();
}
