#include "Merger.h"

Merger::Merger() { }

Merger::~Merger() { }

QList<MergeConflict>& Merger::conflicts() {
	return mConflicts;
}

const QStringList Merger::queries() const {
	return mQueries;
}

void Merger::clear() {
	mConflicts.clear();
	mQueries.clear();
}
