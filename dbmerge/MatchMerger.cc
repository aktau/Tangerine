#include "MatchMerger.h"

using namespace thera;

MatchMerger::MatchMerger() {

}

MatchMerger::~MatchMerger() {

}

void MatchMerger::merge(SQLDatabase *left, SQLDatabase *right) {
	QList<SQLFragmentConf> leftMatches = left->getMatches();
	QList<SQLFragmentConf> rightMatches = right->getMatches();
}
