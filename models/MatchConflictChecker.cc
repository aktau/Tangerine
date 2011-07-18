#include "MatchConflictChecker.h"

#include <QDebug>
#include <QElapsedTimer>

#include "Fragment.h"

using namespace thera;

MatchConflictChecker::MatchConflictChecker(SQLFragmentConf master, QList<SQLFragmentConf> list) : mMaster(master), mList(list) {
	QElapsedTimer t;
	t.start();

	// load fragments (but don't pin them yet)
	foreach (const SQLFragmentConf& c, mList) {
		for (int i = 0; i < IFragmentConf::MAX_FRAGMENTS; ++i) {
			const int fragId = c.mFragments[i];

			if (!mContours.contains(fragId)) {
				const Fragment *fragment = Database::fragment(fragId);

				mContours.insert(fragId, fragment->contour());
			}
		}
	}

	qDebug() << "MatchConflictChecker::MatchConflictChecker: spent" << t.elapsed() << "msec constructing";
}

QList<SQLFragmentConf> MatchConflictChecker::getConflicting() const {
	return filterList(true);
}

QList<SQLFragmentConf> MatchConflictChecker::getNonconflicting() const {
	return filterList(false);
}

QList<SQLFragmentConf> MatchConflictChecker::getProgressiveNonconflicting() const {
	bool conflicting = false;

	QList<thera::SQLFragmentConf> matchList;
	matchList.reserve(mList.size());

	if (!conflicting) matchList << mMaster;

	const int masterTargetId = mMaster.mFragments[IFragmentConf::TARGET];
	const int masterSourceId = mMaster.mFragments[IFragmentConf::SOURCE];

	CPoly2 targetContour = mContours[masterTargetId];
	CPoly2 sourceContour = mContours[masterSourceId];

    targetContour.pin();
    sourceContour.pin();

    QBitArray masterTargetUsed((*targetContour).size());
    QBitArray masterSourceUsed((*sourceContour).size());
    computeOverlap(*sourceContour, *targetContour, mMaster.mXF, masterSourceUsed, masterTargetUsed);

    targetContour.unpin();
    sourceContour.unpin();

    //qDebug() << "Target:" << targetUsed.count(true);
    //qDebug() << "Source:" << sourceUsed.count(true);

    foreach (const SQLFragmentConf& c, mList) {
    	const int targetId = c.mFragments[IFragmentConf::TARGET];
		const int sourceId = c.mFragments[IFragmentConf::SOURCE];

		const bool targetPossibleConflict = targetId == masterTargetId || targetId == masterSourceId;
		const bool sourcePossibleConflict = sourceId == masterTargetId || sourceId == masterSourceId;

    	if (targetPossibleConflict && sourcePossibleConflict) {
    		// this is automatically a conflict because both fragment this match and the master match consist of the same fragments
    		if (conflicting) matchList << c;

    		continue;
    	}

    	CPoly2 tc = mContours[targetId];
		CPoly2 sc = mContours[sourceId];

		tc.pin();
		sc.pin();

		QBitArray targetUsed((*tc).size());
		QBitArray sourceUsed((*sc).size());

		//qDebug("MatchConflictChecker::getConflicting: Match %s with %s -> (%d,%d) and (%d,%d)", c.getTargetId().toAscii().data(), c.getSourceId().toAscii().data(), targetId, sourceId, masterTargetId, masterSourceId);
		//qDebug("Sizes: %d (%d) and %d (%d)", (*tc).size(), targetUsed.count(), (*sc).size(), sourceUsed.count());

		computeOverlap(*sc, *tc, c.mXF, sourceUsed, targetUsed);

		tc.unpin();
		sc.unpin();

		//qDebug("Somehow we did get past it...");

		QBitArray& correspondingMasterUsed = (targetId == masterTargetId || sourceId == masterTargetId) ? masterTargetUsed : masterSourceUsed;
		QBitArray& correspondingUsed = (masterTargetId == targetId || masterSourceId == targetId) ? targetUsed : sourceUsed;

		const int threshold = 5;
		const int minimumUsed = 5; // the minimum amount of points that two fragments have to be close enough
		const int pointsUsed = correspondingUsed.count(true);

		if (pointsUsed < minimumUsed) {
			qDebug() << "MatchConflictChecker::getProgressiveNonconflicting: discarded match" << c.getSourceId() << "<->" << c.getTargetId() << "because only" << pointsUsed << "were used";
			continue;
		}

		if (conflicts(correspondingMasterUsed, correspondingUsed, threshold)) {
			if (conflicting) matchList << c;
		}
		else {
			if (!conflicting) {
				qDebug() << "MatchConflictChecker::getProgressiveNonconflicting: apparently no conflict, adding..." << c.getSourceId() << "<->" << c.getTargetId() << "uses:" << pointsUsed;

				matchList << c;

				qDebug("MatchConflictChecker::getProgressiveNonconflicting: BEFORE USAGE: %d / %d, concordance = %d",
						correspondingMasterUsed.count(true), correspondingMasterUsed.count(),
						(correspondingMasterUsed & correspondingUsed).count(true));
				correspondingMasterUsed |= correspondingUsed;
				qDebug("MatchConflictChecker::getProgressiveNonconflicting: AFTER USAGE: %d / %d", correspondingMasterUsed.count(true), correspondingMasterUsed.count());
			}
		}

		//qDebug("MatchConflictChecker::getConflicting: Match %s with %s\tBAD (%d,%d) and (%d,%d)", c.getTargetId().toAscii().data(), c.getSourceId().toAscii().data(), targetId, sourceId, masterTargetId, masterSourceId);
    }

    // TODO: unpin all

    qDebug() << "Checking conflicts for" << mMaster.getTargetId() << "<->" << mMaster.getSourceId() << mMaster.index()
    		<< "\nWe have" << mList.size() << "candidates and only" << matchList.size() << "are left";

	return matchList;
}

inline QList<SQLFragmentConf> MatchConflictChecker::filterList(bool conflicting) const {
	QList<thera::SQLFragmentConf> matchList;
	matchList.reserve(mList.size());

	if (!conflicting) matchList << mMaster;

	const int masterTargetId = mMaster.mFragments[IFragmentConf::TARGET];
	const int masterSourceId = mMaster.mFragments[IFragmentConf::SOURCE];

	CPoly2 targetContour = mContours[masterTargetId];
	CPoly2 sourceContour = mContours[masterSourceId];

    targetContour.pin();
    sourceContour.pin();

    QBitArray masterTargetUsed((*targetContour).size());
    QBitArray masterSourceUsed((*sourceContour).size());
    computeOverlap(*sourceContour, *targetContour, mMaster.mXF, masterSourceUsed, masterTargetUsed);

    targetContour.unpin();
    sourceContour.unpin();

    //qDebug() << "Target:" << targetUsed.count(true);
    //qDebug() << "Source:" << sourceUsed.count(true);

    foreach (const SQLFragmentConf& c, mList) {
    	const int targetId = c.mFragments[IFragmentConf::TARGET];
		const int sourceId = c.mFragments[IFragmentConf::SOURCE];

		const bool targetPossibleConflict = targetId == masterTargetId || targetId == masterSourceId;
		const bool sourcePossibleConflict = sourceId == masterTargetId || sourceId == masterSourceId;

    	if (targetPossibleConflict && sourcePossibleConflict) {
    		// this is automatically a conflict because both fragment this match and the master match consist of the same fragments
    		if (conflicting) matchList << c;

    		continue;
    	}

    	CPoly2 tc = mContours[targetId];
		CPoly2 sc = mContours[sourceId];

		tc.pin();
		sc.pin();

		QBitArray targetUsed((*tc).size());
		QBitArray sourceUsed((*sc).size());

		//qDebug("MatchConflictChecker::getConflicting: Match %s with %s -> (%d,%d) and (%d,%d)", c.getTargetId().toAscii().data(), c.getSourceId().toAscii().data(), targetId, sourceId, masterTargetId, masterSourceId);
		//qDebug("Sizes: %d (%d) and %d (%d)", (*tc).size(), targetUsed.count(), (*sc).size(), sourceUsed.count());

		computeOverlap(*sc, *tc, c.mXF, sourceUsed, targetUsed);

		//qDebug("Somehow we did get past it...");

		const QBitArray& correspondingMasterUsed = (targetId == masterTargetId || sourceId == masterTargetId) ? masterTargetUsed : masterSourceUsed;
		const QBitArray& correspondingUsed = (masterTargetId == targetId || masterSourceId == targetId) ? targetUsed : sourceUsed;

		const int threshold = 10;

		if (conflicts(correspondingMasterUsed, correspondingUsed, threshold)) {
			if (conflicting) matchList << c;
		}
		else {
			if (!conflicting) matchList << c;
		}

		//qDebug("MatchConflictChecker::getConflicting: Match %s with %s\tBAD (%d,%d) and (%d,%d)", c.getTargetId().toAscii().data(), c.getSourceId().toAscii().data(), targetId, sourceId, masterTargetId, masterSourceId);
    }

    // TODO: unpin all

    qDebug() << "Checking conflicts for" << mMaster.getTargetId() << "<->" << mMaster.getSourceId() << mMaster.index()
    		<< "\nWe have" << mList.size() << "candidates and only" << matchList.size() << "are left";

	return matchList;
}

inline void MatchConflictChecker::computeOverlap(const Poly2& source, const Poly2& target, const XF& xf, QBitArray& sourceUsed, QBitArray& targetUsed) const {
	// anything marked as used will be preserved as used
	assert(source.size() == sourceUsed.size());
	assert(target.size() == targetUsed.size());

	const float maxsqdist = 1;

	vec2 mn = target[0], mx = target[0];
	for (int j = 1, jj = target.size(); j < jj; ++j) {
		mn = std::min(mn, target[j]);
		mx = std::max(mx, target[j]);
	}

	//std::cerr << mn << " | " << mx << std::endl;

	for (register int i = 0, ii = source.size(), jj = target.size(); i < ii; ++i) {
		// assuming planar transform
		const vec2 p(
			xf[0] * source[i][0] + xf[4] * source[i][1] + xf[12],
			xf[1] * source[i][0] + xf[5] * source[i][1] + xf[13]
		);

		/* early fail checks */
		if (p[0] < mn[0] - maxsqdist) continue;
		if (p[0] > mx[0] + maxsqdist) continue;
		if (p[1] < mn[1] - maxsqdist) continue;
		if (p[1] > mx[1] + maxsqdist) continue;

		//qDebug() << "passed first continue tests";

		for (register int j = 0; j < jj; ++j) {
			if (sourceUsed.testBit(i) && targetUsed.testBit(j)) continue;

			if (dist2(p, target[j]) < maxsqdist) {
				//sourceUsed[i] = 1;
				//targetUsed[j] = 1;

				//qDebug("%f < %f < %f => %s", mn[0] - maxsqdist, p[0], mx[0] + maxsqdist, (mn[0] - maxsqdist < p[0] && p[0] < mx[0] + maxsqdist) ? "YES" : "NO");
				//qDebug("%f < %f < %f => %s", mn[1] - maxsqdist, p[1], mx[1] + maxsqdist, (mn[1] - maxsqdist < p[1] && p[1] < mx[1] + maxsqdist) ? "YES" : "NO");
				//qDebug() << "---------------------------------";

				sourceUsed.setBit(i);
				targetUsed.setBit(j);

				break;
			}
		}
	}
}

inline bool MatchConflictChecker::conflicts(const QBitArray& a, const QBitArray& b, int threshold) const {
	  assert(a.size() == b.size());

	  /*
	  for (register int i = 0, ii = a.size(); i < ii && threshold != 0; ++i) {
		  threshold -= (a.testBit(i) & b.testBit(i));
	  }

	  qDebug("threshold = %d", threshold);

	  return !threshold;
	  */

	  // alternatively and might actually be faster (look at the Qt source, the operators and count() have some neat bithacks:
	  return (a & b).count(true) > threshold;
}
