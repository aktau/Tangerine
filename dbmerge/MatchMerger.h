#ifndef MATCHMERGER_H_
#define MATCHMERGER_H_

#include "Merger.h"

#include <QSet>
#include <QHash>

class MatchMerger: public Merger {
		Q_OBJECT

	public:
		MatchMerger();
		virtual ~MatchMerger();

	public:
		void merge(SQLDatabase *left, SQLDatabase *right);

	private:
		typedef QPair<int, int> IntPair;
		typedef QSet<const thera::SQLFragmentConf *> FragConfList;
		typedef QHash< IntPair, FragConfList > FragmentMap;
		typedef QHash<int, const thera::SQLFragmentConf *> IndexMap;

	private:
		// will return -1 if none of matches in the list is deemed to be a duplicate
		int idOfIdenticalMatch(const thera::SQLFragmentConf& conf , const FragConfList& list, float threshold = 0.001f) const;
};

#endif /* MATCHMERGER_H_ */
