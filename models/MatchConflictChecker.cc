#include "MatchConflictChecker.h"

#include <QDebug>
#include <QElapsedTimer>

#include "Fragment.h"

using namespace thera;

MatchConflictChecker::MatchConflictChecker(SQLFragmentConf master, QList<SQLFragmentConf> list) : mMaster(master), mList(list) { }

QList<SQLFragmentConf> MatchConflictChecker::getConflicting() const {
	qDebug() << "Checking conflicts for" << mMaster.getTargetId() << "<->" << mMaster.getSourceId() << mMaster.index();

    const Fragment *targetFragment = Database::fragment(mMaster.mFragments[IFragmentConf::TARGET]);
    const Fragment *sourceFragment = Database::fragment(mMaster.mFragments[IFragmentConf::SOURCE]);

    CPoly2 targetContour = targetFragment->contour();
    CPoly2 sourceContour = sourceFragment->contour();

    targetContour.pin();
    sourceContour.pin();

    //QBitArray targetUsed((*targetContour).size());
    //QBitArray sourceUsed((*sourceContour).size());

    QElapsedTimer t;
	t.start();
	for (int k = 0; k < 1000; ++k) {
		QBitArray targetUsed((*targetContour).size());
		QBitArray sourceUsed((*sourceContour).size());
		computeOverlap(*sourceContour, *targetContour, mMaster.mXF, sourceUsed, targetUsed);
	}
	qDebug() << "Found" << 0 << "overlaps in" << t.elapsed() << "msec";

    targetContour.unpin();
    sourceContour.unpin();

    //qDebug() << "Target:" << targetUsed.count(true);
    //qDebug() << "Source:" << sourceUsed.count(true);

	return mList;
}

QList<SQLFragmentConf> MatchConflictChecker::getNonconflicting() const {
	return mList;
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
				targetUsed.setBit(i);

				break;
			}
		}
	}
}
