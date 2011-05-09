#include "MatchModel.h"

#include <QDebug>
#include <QElapsedTimer>

using namespace thera;

MatchModel::MatchModel(SQLDatabase *db) : mDb(db) {
	if (mDb == NULL) {
		qDebug() << "MatchModel::MatchModel: passed database was NULL, this will lead to trouble";
	}
	else {
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

bool MatchModel::isValidIndex(int index) const {
	return (index >= 0) && (index < size());
}

int MatchModel::size() const {
	return mMatches.size();
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
	if (mFilter != pattern) {
		mFilter = pattern;

		populateModel();

		emit modelChanged();
	}
}

thera::IFragmentConf& MatchModel::get(int index) {
	return mMatches[index];
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
	return mFilter;
}

void MatchModel::populateModel() {
	mMatches = mDb->getMatches(mSortField, mSortOrder, mFilter);
}

void MatchModel::resetSort() {
	mSortField = QString();
	mSortOrder = Qt::AscendingOrder;
}

void MatchModel::resetFilter() {
	mFilter = QString();
}

void MatchModel::databaseModified() {
	resetSort();
	resetFilter();

	populateModel();

	emit modelChanged();

	qDebug() << "MatchModel::databaseModified: apparently, the database was modified, now available:" << size();
}
