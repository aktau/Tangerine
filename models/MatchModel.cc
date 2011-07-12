#include "MatchModel.h"

#include <QDebug>
#include <QElapsedTimer>

#include "SQLFilter.h"

using namespace thera;

MatchModel::MatchModel(SQLDatabase *db) : mDb(db), mFilter(db), mRealSize(0), mWindowSize(20), mWindowBegin(0), mWindowEnd(0) {
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

void MatchModel::setWindowSize(int size) {
	assert(size > 0);

	mWindowSize = size;
}

void MatchModel::requestWindow(int windowIndex) {
	mWindowBegin = windowIndex * mWindowSize;
	mWindowEnd = (windowIndex + 1) * mWindowSize;

	populateModel();
}

bool MatchModel::isValidIndex(int index) const {
	return (index >= 0) && (index < size());
}

int MatchModel::size() const {
	return mRealSize;
}

void MatchModel::sort(const QString& field, Qt::SortOrder order) {
	// TODO: sanity check on field
	if (!mDb->matchHasField(field)) {
		qDebug() << "MatchModel::sort: Field" << field << "doesn't exist";
	}
	else if (mSortField != field || mSortOrder != order) {
		mSortField = field;
		mSortOrder = order;

		// ask the database for a new, sorted, list
		populateModel();

		emit orderChanged();
	}
}

void MatchModel::filter(const QString& pattern) {
	if (mNameFilter != pattern) {
		if (!pattern.isEmpty()) {
			QString normalizedFilter = pattern;

			if (!normalizedFilter.startsWith('*')) normalizedFilter.prepend("*");
			if (!normalizedFilter.endsWith('*')) normalizedFilter.append("*");

			normalizedFilter = QString(normalizedFilter).replace("*","%").replace("?","_");

			mFilter.setFilter("matchmodel_names", QString("source_name || target_name LIKE '%1' OR target_name || source_name LIKE '%1'").arg(normalizedFilter));
		}
		else {
			mFilter.removeFilter("matchmodel_names");
		}

		mNameFilter = pattern;

		requestRealSize();
		populateModel();

		emit modelChanged();
	}
}

void MatchModel::genericFilter(const QString& key, const QString& filter) {
	if (!filter.isEmpty()) {
		mFilter.setFilter(key, filter);
	}
	else {
		mFilter.removeFilter(key);
	}

	requestRealSize();
	populateModel();

	emit modelChanged();
}

thera::IFragmentConf& MatchModel::get(int index) {
	if (index < mWindowBegin || index > mWindowEnd) {
		// if the index is outside of the window, request another window in which it fits
		requestWindow(index / mWindowSize);
	}

	return mMatches[index % mWindowSize];
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
	return mNameFilter;
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

	mMatches = mDb->getMatches(mSortField, mSortOrder, mFilter, mWindowBegin, mWindowSize);
	mWindowEnd = mWindowBegin + mWindowSize - 1;

	//mMatches = mDb->getMatches(mSortField, mSortOrder, mFilter);
	//mRealSize = mMatches.size();

	qDebug() << "MatchModel::populateModel: Done repopulating model," << timer.elapsed() << "milliseconds";
}

void MatchModel::requestRealSize() {
	QElapsedTimer timer;
	timer.start();

	mRealSize = mDb->getNumberOfMatches(mFilter);

	qDebug() << "MatchModel::populateModel: Get # of matches," << timer.elapsed() << "milliseconds [result" << mRealSize << "matches]";
}

void MatchModel::resetWindow() {
	mWindowBegin = 0;
	mWindowEnd = mWindowBegin + mWindowSize - 1;
}

/**
 * resets without firing signals
 */
void MatchModel::resetSort() {
	mSortField = QString();
	mSortOrder = Qt::AscendingOrder;
}

/**
 * resets without firing signals
 */
void MatchModel::resetFilter() {
	mNameFilter = QString();
	mFilter.clear();
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
