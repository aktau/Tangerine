#include "Merger.h"

Merger::Merger() : mMapper(NULL) { }

Merger::~Merger() {
	clear();
}

void Merger::setMapper(MergeMapper *mapper) {
	mMapper = mapper;
}

const QList<MergeItem *>& Merger::items() {
	return mItems;
}

/*
const QList<MergeConflict>& Merger::conflicts() {
	return mConflicts;
}
*/

const QStringList Merger::queries() const {
	return mQueries;
}

void Merger::clear() {
	qDeleteAll(mItems);
	mItems.clear();
	mQueries.clear();
}
