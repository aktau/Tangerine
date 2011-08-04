#include "AttributeMerger.h"

#include <QtAlgorithms>

#include <AttributeMergeItem.h>

/*
struct HistoryLessThan : public std::binary_function<HistoryRecord *, HistoryRecord *, bool> {
    bool operator()(const HistoryRecord *lhs, const HistoryRecord *rhs) const {
        return lhs->timestamp < rhs->timestamp;
    }
};
*/

AttributeMerger::AttributeMerger() {

}

AttributeMerger::~AttributeMerger() {
	qDebug() << "AttributeMerger::~AttributeMerger: ran";
}

void AttributeMerger::merge(SQLDatabase *left, SQLDatabase *right) {
	assert(mMapper != NULL);

	// take history into account, only when conflict I guess

	// fetch all matches on the left side (which is supposed to contain all the matches that we have to possibly merge in)
	// reverse mapper useful??
	//QList<SQLFragmentConf> leftMatches = left->getMatches();
	// not really necessary, we just need to get attributes and map them with the mapper!

	// 1) get history, create map based on match ID
	// 2) filter out unmerged matches

	const QSet<QString>& attributes = right->realMatchFields();

	IdToHistoryMap leftIdToHistoryMap;
	IdToHistoryMap rightIdToHistoryMap;

	foreach (const QString& attribute, attributes) {
		QList<HistoryRecord> leftHistory = left->getHistory(attribute, "timestamp", Qt::AscendingOrder);
		QList<HistoryRecord> rightHistory = right->getHistory(attribute, "timestamp", Qt::AscendingOrder);

		leftIdToHistoryMap.clear();
		rightIdToHistoryMap.clear();

		fillHistoryMap(leftIdToHistoryMap, leftHistory, false); // non-mapped
		fillHistoryMap(rightIdToHistoryMap, rightHistory, true); // mapped

		//qDebug() << "AttributeMerger::merge: master map size =" << leftIdToHistoryMap.size() << ", slave map size =" << rightIdToHistoryMap.size();

		// TODO: assert / sanity check to see if the list contains duplicate pointers
		{
			QList<AttributeRecord> leftAttributes = left->getAttribute(attribute);
			QList<AttributeRecord> rightAttributes = right->getAttribute(attribute);

			qDebug() << "AttributeMerger::merge: WARNING left: " << attribute << "->" << leftAttributes.size() << "vs" << leftIdToHistoryMap.size();
			qDebug() << "AttributeMerger::merge: WARNING right: " << attribute  << "->" << rightAttributes.size() << "vs" << rightIdToHistoryMap.size();
		}

		mergeAttribute(attribute, leftIdToHistoryMap, rightIdToHistoryMap);

		// merge an attribute here
	}
}

void AttributeMerger::execute(SQLDatabase *left, MergeMapper *mapper) {
	assert(left != NULL && mapper != NULL);

	// order transactions correctly (those without ID reassignment first!)
	left->transaction();
	foreach (MergeItem *item, mItems) {
		if (!item->isDone()) {
			if (!item->execute(left, mapper)) {
				qDebug() << "AttributeMerger::execute: item" << item->message() << "did not execute properly";
			}
		}
	}
	left->commit();
}

inline void AttributeMerger::mergeAttribute(const QString& attribute, const IdToHistoryMap& leftHistoryMap, const IdToHistoryMap& rightHistoryMap) {
	foreach (int id, rightHistoryMap.keys()) {
		qDebug() << "AttributeMerger::mergeAttribute: merging attribute" << attribute.toUpper() << "for ID" << id << "\n-----------------------------";

		IdToHistoryMap::const_iterator leftIterator = leftHistoryMap.constFind(id);
		IdToHistoryMap::const_iterator rightIterator = rightHistoryMap.constFind(id);

		if (rightIterator == rightHistoryMap.constEnd()) {
			qDebug() << "AttributeMerger::mergeAttribute: this should never happen" << id;

			continue;
		}

		//qDebug() << "got here";

		if (leftIterator == leftHistoryMap.constEnd()) {
			// just merge in
			qDebug() << "No history for" << id << "->" << attribute << "in master, merging...";

			mItems << new AttributeMergeItem(id, HistoryList(), *rightIterator);
			MostRecentAction action;
			action.visit(mItems.last());
		}
		else {
			mItems << new AttributeMergeItem(id, *leftIterator, *rightIterator);
			ChooseHistoryAction action;
			action.visit(mItems.last());
		}
	}
}

void AttributeMerger::fillHistoryMap(IdToHistoryMap& map, QList<HistoryRecord>& historyList, bool mapIndices) {
	for (int i = 0; i < historyList.size(); ++i) {
		HistoryRecord& histRecord = historyList[i];

		// if the attribute wasn't mapped to -1
		int realId = (mapIndices) ? mMapper->get(MergeMapper::MATCH_ID, histRecord.matchId, -2) : histRecord.matchId;
		int realUser = (mapIndices) ? mMapper->get(MergeMapper::USER_ID, histRecord.userId, -2) : histRecord.userId;

		if (realId == -1) {
			qDebug() << "AttributeMerger::fillHistoryMap: match explicitly NOT merged. Attribute name =" << "?" << "and match id =" << histRecord.matchId;
		}
		else if (realId == -2) {
			qDebug() << "AttributeMerger::fillHistoryMap: No mapping was found for this match... strange. Attribute name =" << "?" << "and match id =" << histRecord.matchId;

			map[histRecord.matchId] << histRecord;
		}
		else {
			if (histRecord.matchId != realId) {
				qDebug() << "AttributeMerger::fillHistoryMap: encountered a mapped match id" << histRecord.matchId << "!=" << realId;

				histRecord.matchId = realId;
			}

			if (histRecord.userId != realUser) {
				qDebug() << "AttributeMerger::fillHistoryMap: encountered a mapped user id" << histRecord.userId << "!=" << realUser;

				histRecord.userId = (realUser >= 0) ? realUser : histRecord.userId;
			}

			// should create key if it didn't exist already
			map[histRecord.matchId] << histRecord;
		}
	}
}
