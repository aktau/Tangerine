#include "SQLFilter.h"

#include <QDebug>

#include "SQLDatabase.h"

SQLFilter::SQLFilter() : mDb(NULL) {

}

SQLFilter::SQLFilter(SQLDatabase *db) : mDb(NULL) {
	setDatabase(db);
}

SQLFilter::~SQLFilter() {

}

void SQLFilter::setDatabase(SQLDatabase *db) {
	if (db == NULL) {
		qDebug() << "SQLFilter::setDatabase: attempted to set NULL database";

		return;
	}

	mDb = db;
}

QStringList SQLFilter::dependencies() const {
	return QStringList::fromSet(mDependencies);
}

QStringList SQLFilter::clauses() const {
	return mFilters.values();
}

bool SQLFilter::isEmpty() const {
	return mFilters.isEmpty();
}

bool SQLFilter::hasFilter(const QString& key) const {
	return mFilters.contains(key);
}

bool SQLFilter::hasFilter(const QString& key, const QString& filter) const {
	if (mFilters.contains(key) && filter == mFilters.value(key)) {
		return true;
	}

	return false;
}

void SQLFilter::setFilter(const QString& key, const QString& filter) {
	if (mDb == NULL) {
		qDebug() << "SQLFilter::addFilter: can't add filter while database is NULL, please use setDatabase() first";

		return;
	}

	mFilters.insert(key, filter);

	updateDependencyInfo();
}

void SQLFilter::removeFilter(const QString& key) {
	mFilters.remove(key);

	updateDependencyInfo();
}

void SQLFilter::clear() {
	mFilters.clear();
	mDependencies.clear();
}

bool SQLFilter::operator==(const SQLFilter& other) const {
	return mDb == other.mDb && mFilters == other.mFilters;
}

bool SQLFilter::operator!=(const SQLFilter& other) const {
	return !(*this == other);
}

/**
 * This may look a bit brute-force-ish, but this is not really performance critical and
 * it works out to be much less error prone and simpler than partial updates
 */
void SQLFilter::updateDependencyInfo() {
	assert(mFilters.isEmpty() || mDb != NULL);

	mDependencies.clear();

	// yes we could streamline this loop a little by continue'ing once we've added a certain field
	// but that's low priority
	foreach (const QString& field, mDb->matchFields()) {
		foreach (const QString& filter, mFilters.values()) {
			// for now we choose to be case-sensitive, because some SQL systems are
			if (filter.contains(field)) {
				mDependencies << field;
			}
		}
	}
}
