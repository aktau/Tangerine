#ifndef MATCHMERGER_H_
#define MATCHMERGER_H_

#include "Merger.h"

class MatchMerger: public Merger {
		Q_OBJECT

	public:
		MatchMerger();
		virtual ~MatchMerger();

	public:
		void merge(SQLDatabase *left, SQLDatabase *right);
};

#endif /* MATCHMERGER_H_ */