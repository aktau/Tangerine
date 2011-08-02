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
