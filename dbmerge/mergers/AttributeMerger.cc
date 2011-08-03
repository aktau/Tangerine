#include "AttributeMerger.h"

// yea, Qt doesn't really provide these things exactly and I don't feel like rolling my own buggy versions
// I'm paying the cost for the STL already anyway
//#include <set>
//#include <algorithm>

#include <QtAlgorithms>

#define MERGE_DEBUG 1

struct HistoryLessThan : public std::binary_function<HistoryRecord *, HistoryRecord *, bool> {
    bool operator()(const HistoryRecord *lhs, const HistoryRecord *rhs) const {
        return lhs->timestamp < rhs->timestamp;
    }
};

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

		// TODO: assert / sanity check to see if the list contain duplicate pointers
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
			qDebug() << "AttributeMerger::mergeAttribute: this should never happen";
		}

		if (leftIterator == leftHistoryMap.constEnd()) {
			// just merge in
			qDebug() << "No history for" << id << "->" << attribute << "in master, merging...";
		}
		else {
			const HistoryPointerList& leftHistory = leftIterator.value();
			const HistoryPointerList& rightHistory = rightIterator.value();

			const HistoryRecord *leftMostRecent = leftHistory.last();
			const HistoryRecord *rightMostRecent = rightHistory.last();

			if (*leftMostRecent == *rightMostRecent) {
			#ifdef MERGE_DEBUG
				qDebug() << "RESULT: Both were equal, not doing anything: " << leftMostRecent->toString();
				qDebug() << "----------------------------------";
			#endif

				continue;
			}

			#if MERGE_DEBUG == 2
			qDebug() << "Master history";
			int i = 0;
			foreach (const HistoryRecord *r, leftHistory) {
				qDebug() << ++i << "-" << r->toString();
			}

			qDebug() << "Slave history";
			i = 0;
			foreach (const HistoryRecord *r, rightHistory) {
				qDebug() << ++i << "-" << r->toString();
			}
			#endif
			HistoryLessThan hLess;
			HistoryPointerList::const_iterator found = qBinaryFind(rightHistory.begin(), rightHistory.end(), leftMostRecent, hLess);

			// look for shared history, conflict, resolve
			if (found != rightHistory.end()) {
				if (*leftMostRecent == **found) {
					qDebug() << "RESULT: Found master history inside of slave: supposed to merge!";
				}
				else {
					qDebug() << "RESULT: Freak accident #1: exactly the same timestamp but not the same, what to do?:" << leftMostRecent->toString() << "vs" << (*found)->toString();

					//while (++found != rightHistory.end()) { ... }
				}
			}
			else {
				qDebug() << "RESULT: Did NOT find master history inside of slave";

				found = qBinaryFind(leftHistory.begin(), leftHistory.end(), rightMostRecent, hLess);
				if (found != leftHistory.end()) {
					if (*rightMostRecent == **found) {
						qDebug() << "RESULT: Found last slave update inside of master history: do not merge, master is more recent!";
					}
					else {
						qDebug() << "RESULT: Freak accident #2: exactly the same timestamp but not the same, what to do?:" << rightMostRecent->toString() << "vs" << (*found)->toString();
					}
				}
				else {
					qDebug() << "RESULT: Histories diverged or were never the same in the first place: conflict resolution magic here";
				}
			}

			qDebug() << "----------------------------------";
		}
	}

	//mItems << fixedIdList;
}

void AttributeMerger::fillHistoryMap(IdToHistoryMap& map, QList<HistoryRecord>& historyList, bool mapIndices) {
	for (int i = 0; i < historyList.size(); ++i) {
	//foreach (HistoryRecord& histRecord, historyList) {
		HistoryRecord& histRecord = historyList[i];

		// if the attribute wasn't mapped to -1
		int realId = (mapIndices) ? mMapper->get(MergeMapper::MATCH_ID, histRecord.matchId, -2) : histRecord.matchId;
		int realUser = (mapIndices) ? mMapper->get(MergeMapper::USER_ID, histRecord.userId, -2) : histRecord.userId;

		if (realId == -1) {
			qDebug() << "AttributeMerger::fillHistoryMap: match explicitly NOT merged. Attribute name =" << "?" << "and match id =" << histRecord.matchId;
		}
		else if (realId == -2) {
			qDebug() << "AttributeMerger::fillHistoryMap: No mapping was found for this match... strange. Attribute name =" << "?" << "and match id =" << histRecord.matchId;

			map[histRecord.matchId] << &histRecord;
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
			map[histRecord.matchId] << &histRecord;
		}
	}
}


/*
const HistoryPointerList& leftHistory = leftIterator.value();
const HistoryPointerList& rightHistory = rightIterator.value();

const HistoryRecord *leftMostRecent = leftHistory.last();
const HistoryRecord *rightMostRecent = rightHistory.last();

if (*leftMostRecent == *rightMostRecent) {
#ifdef MERGE_DEBUG
	qDebug() << "RESULT: Both were equal, not doing anything: " << leftMostRecent->toString();
	qDebug() << "----------------------------------";
#endif

	continue;
}

#if MERGE_DEBUG == 2
qDebug() << "Master history";
int i = 0;
foreach (const HistoryRecord *r, leftHistory) {
	qDebug() << ++i << "-" << r->toString();
}

qDebug() << "Slave history";
i = 0;
foreach (const HistoryRecord *r, rightHistory) {
	qDebug() << ++i << "-" << r->toString();
}
#endif
HistoryLessThan hLess;
HistoryPointerList::const_iterator found = qBinaryFind(rightHistory.begin(), rightHistory.end(), leftMostRecent, hLess);

// look for shared history, conflict, resolve
if (found != rightHistory.end()) {
	if (*leftMostRecent == **found) {
		qDebug() << "RESULT: Found master history inside of slave: supposed to merge!";
	}
	else {
		qDebug() << "RESULT: Freak accident #1: exactly the same timestamp but not the same, what to do?:" << leftMostRecent->toString() << "vs" << (*found)->toString();

		//while (++found != rightHistory.end()) { ... }
	}
}
else {
	qDebug() << "RESULT: Did NOT find master history inside of slave";

	found = qBinaryFind(leftHistory.begin(), leftHistory.end(), rightMostRecent, hLess);
	if (found != leftHistory.end()) {
		if (*rightMostRecent == **found) {
			qDebug() << "RESULT: Found last slave update inside of master history: do not merge, master is more recent!";
		}
		else {
			qDebug() << "RESULT: Freak accident #2: exactly the same timestamp but not the same, what to do?:" << rightMostRecent->toString() << "vs" << (*found)->toString();
		}
	}
	else {
		qDebug() << "RESULT: Histories diverged or were never the same in the first place: conflict resolution magic here";
	}
}

qDebug() << "----------------------------------";
*/
