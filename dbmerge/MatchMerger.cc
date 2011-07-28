#include "MatchMerger.h"

using namespace thera;

MatchMerger::MatchMerger() {

}

MatchMerger::~MatchMerger() {

}

void MatchMerger::merge(SQLDatabase *left, SQLDatabase *right) {
	/*
	for (int i = 0; i < 5; ++i) {
		QList<SQLFragmentConf> leftMatches = left->getMatches();
		QList<SQLFragmentConf> rightMatches = right->getMatches();
	}
	*/

	QList<SQLFragmentConf> leftMatches = left->getMatches();
	QList<SQLFragmentConf> rightMatches = right->getMatches();

	typedef QHash<int, const SQLFragmentConf *> IndexMap;
	IndexMap leftMap;
	leftMap.reserve(leftMatches.size());

	QElapsedTimer timer;
	timer.start();

	foreach (const SQLFragmentConf& leftConf, leftMatches) {
		leftMap.insert(leftConf.index(), &leftConf);
	}

	qDebug() << "MatchMerger::merge: constructing map of size" << leftMap.size() << "took" << timer.restart() << "msec";

	// if the ID's match, make sure the rest matches as well, if not, re-assign ID
	IndexMap::const_iterator i, end = leftMap.constEnd();
	foreach (const SQLFragmentConf& rightConf, rightMatches) {
		if ((i = leftMap.constFind(rightConf.index())) != end) {
			qDebug() << "FOUND CONFLICT" << rightConf.index()
				<< "left:" << i.value()->getSourceId() << "<->" << i.value()->getTargetId()
				<< "-- right:" << rightConf.getSourceId() << "<->" << rightConf.getTargetId();
		}
	}

	/*
	foreach (const SQLFragmentConf& leftConf, leftMatches) {
		foreach (const SQLFragmentConf& rightConf, rightMatches) {

		}
	}
	*/
}
