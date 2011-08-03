#ifndef ATTRIBUTEMERGER_H_
#define ATTRIBUTEMERGER_H_

#include "Merger.h"

#include <QHash>

#include "SQLRawTheraRecords.h"

class AttributeMerger : public Merger {
		Q_OBJECT

	public:
		AttributeMerger();
		virtual ~AttributeMerger();

	public:
		void merge(SQLDatabase *left, SQLDatabase *right);
		void execute(SQLDatabase *left, MergeMapper *mapper);

	private:
		//typedef QList<HistoryRecord *> HistoryPointerList;
		//typedef QHash<int, HistoryPointerList> IdToHistoryMap;
		typedef QList<const HistoryRecord *> HistoryPointerList;
		typedef QHash<int, HistoryPointerList> IdToHistoryMap;

	private:
		void mergeAttribute(const QString& attribute, const IdToHistoryMap& leftHistoryMap, const IdToHistoryMap& rightHistoryMap);
		void fillHistoryMap(IdToHistoryMap& map, QList<HistoryRecord>& historyList, bool mapIndices = false);
};

#endif /* ATTRIBUTEMERGER_H_ */
