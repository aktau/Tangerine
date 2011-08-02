#include "Merger.h"

Merger::Merger() : mMapper(NULL) { }

Merger::~Merger() {
	clear();
}

void Merger::setMapper(MergeMapper *mapper) {
	if (!mapper) qDebug() << "Merger::setMapper: a NULL mapper was set, don't be surprised if a segmentation fault occurs";

	mMapper = mapper;
}

const QList<MergeItem *>& Merger::items() {
	return mItems;
}

void Merger::clear() {
	qDeleteAll(mItems);
	mItems.clear();
}

bool Merger::isResolved() const {
	bool allResolved = true;

	foreach (const MergeItem *item, mItems) {
		if ((allResolved = item->isResolved()) == false) break;
	}

	return allResolved;
}

bool Merger::isDone() const {
	bool allDone = true;

	foreach (const MergeItem *item, mItems) {
		if ((allDone = item->isDone()) == false) break;
	}

	return allDone;
}
