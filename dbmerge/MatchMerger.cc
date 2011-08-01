#include "MatchMerger.h"

#include <limits>

#include "MergeItemSubclasses.h"

using namespace thera;

#define QT_USE_FAST_CONCATENATION
#define QT_USE_FAST_OPERATOR_PLUS
#define MERGE_DEEP_CHECK

template <class T>
static inline float frobeniusf(const XForm<T> &xf) {
	float norm = 0.0f;

	for (int i = 0; i < 16; ++i) {
		norm += xf[i]*xf[i];
	}

	return sqrt(norm);
}

MatchMerger::MatchMerger() {

}

MatchMerger::~MatchMerger() {
	qDebug() << "MatchMerger::~MatchMerger: ran";
}

void MatchMerger::merge(SQLDatabase *left, SQLDatabase *right) {
	assert(mMapper != NULL);

	QList<SQLFragmentConf> leftMatches = left->getMatches();
	QList<SQLFragmentConf> rightMatches = right->getMatches();

	QElapsedTimer timer;
	timer.start();

	FragmentMap leftMap;
	leftMap.reserve(leftMatches.size());

	// TODO: convert to QSet instead of QMap/QHash, the value is never used anyway
	IndexMap leftIndices;
	leftIndices.reserve(leftMatches.size());

	foreach (const SQLFragmentConf& leftConf, leftMatches) {
		leftIndices.insert(leftConf.index(), &leftConf);
	}

	qDebug() << "MatchMerger::merge: constructing index map of size" << leftIndices.size() << "took" << timer.restart() << "msec";

	foreach (const SQLFragmentConf& leftConf, leftMatches) {
		int min = qMin(leftConf.mFragments[IFragmentConf::SOURCE], leftConf.mFragments[IFragmentConf::TARGET]);
		int max = qMax(leftConf.mFragments[IFragmentConf::SOURCE], leftConf.mFragments[IFragmentConf::TARGET]);

		IntPair pair(min, max);

		FragmentMap::iterator i = leftMap.find(pair);

		if (i != leftMap.end()) {
			i.value() << &leftConf;

			//qDebug() << "List existed already, current list size for pair" << pair << "=" << i.value().size();
		}
		else {
			leftMap.insert(pair, FragConfList() << &leftConf);
		}
	}

	qDebug() << "MatchMerger::merge: constructing fragment map of size" << leftMap.size() << "took" << timer.restart() << "msec";

	FragmentMap::const_iterator end = leftMap.constEnd();
	foreach (const SQLFragmentConf& rightConf, rightMatches) {
		int min = qMin(rightConf.mFragments[IFragmentConf::SOURCE], rightConf.mFragments[IFragmentConf::TARGET]);
		int max = qMax(rightConf.mFragments[IFragmentConf::SOURCE], rightConf.mFragments[IFragmentConf::TARGET]);

		IntPair pair(min, max);

		FragmentMap::const_iterator i = leftMap.constFind(pair);

		// if the current match in the right database has the same fragments as a match
		// on the left database, look into it to see if it either is the same match (ID merging)
		if (i != end) {
			//const FragConfList& list = i.value();

			int id = idOfIdenticalMatch(rightConf, i.value());

			if (id != -1 && rightConf.index() != id) {
				// this means that the current item on the right matches one on the right
				// but doesn't have the same ID, we'll just map the right ID to the left ID
				// so that future mergers can recognize that both match id's refer to
				// the same object

				qDebug("MatchMerger::merge: right id (%d) is from now on equal to left id (%d)", rightConf.index(), id);
				mMapper->addMapping(MergeMapper::MATCH_ID, rightConf.index(), id);

				continue;
			}
			else {
				// this means the current item on the left matches on on the right
				// and has the same ID, nothing needs to happen
				continue;
			}
		}

		// this means the current item from right doesn't match any in left
		// if the ID assigned to it is occupied, we have to pick a new one
		if (leftIndices.contains(rightConf.index())) {
			// it's occupied, we have to pick a new one
			qDebug() << "MatchMerger::merge: ID conflict, reassigning" << rightConf.index() << "to another ID. right (source <-> conf) = " << rightConf.getSourceId() << "<->" << rightConf.getTargetId();
			mMapper->addMapping(MergeMapper::MATCH_ID, rightConf.index(), 0xDEADBEEF);

			QString xf;
			for (int col = 0; col < 4; ++col) {
				for (int row = 0; row < 4; ++row) {
					xf += QString("%1 ").arg(rightConf.mXF[4 * row + col], 0, 'e', 20);
				}
			}

			mItems << new MatchMergeItem(rightConf.getSourceId(), rightConf.getTargetId(), xf);
			//AssignIdAction action(0xDEADBEEF);
			//action.visit(mItems.last());
		}
	}
}

inline int MatchMerger::idOfIdenticalMatch(const thera::SQLFragmentConf& conf , const FragConfList& list, float threshold) const {
	// compare based on XF, the fragments are already the same
	// TODO: detect the case in which fragments are reversed and then complain about it (might be slow and shouldn't happen though)
	// TODO: remove "norms", it's just for debugging purposes

	const XF baseXF = conf.mXF;

	QList<float> norms;
	norms.reserve(list.size());

	float smallestNorm = std::numeric_limits<float>::max();
	int id = -1;

	foreach (const SQLFragmentConf *possibleConf, list) {
		norms << frobeniusf(baseXF - possibleConf->mXF);

		if (norms.last() < smallestNorm) {
			smallestNorm = norms.last();
			id = possibleConf->index();
		}
	}

	QStringList ids;
	ids << QString("<%1>").arg(conf.index());
	foreach (const SQLFragmentConf *possibleConf, list) ids << QString::number(possibleConf->index());
	qDebug() << "norms =" << norms << ids.join(" - ");

	return (smallestNorm < threshold) ? id : -1;
}
