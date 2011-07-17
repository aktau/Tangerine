#include "MatchConflictChecker.h"

#include <QDebug>
#include <QElapsedTimer>

#include "Fragment.h"

using namespace thera;

// http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
template <typename T> int nsign(T val) {
    return (val > T(0)) - (val < T(0));
}

MatchConflictChecker::MatchConflictChecker(SQLFragmentConf master, QList<SQLFragmentConf> list) : mMaster(master), mList(list) { }

static void compute_contour_overlaps(const thera::Poly2 &src, const thera::Poly2 &tgt, const thera::XF &xf, std::vector<bool> &src_used, std::vector<bool> &tgt_used) {
	// anything marked as used will be preserved as used
	assert(src_used.size() == src.size());
	assert(tgt_used.size() == tgt.size());
	const float maxsqdist = 1;

	vec2 mn = tgt[0], mx = tgt[0];
	for (size_t j = 1; j < tgt.size(); j++) {
		mn = std::min(mn, tgt[j]);
		mx = std::max(mx, tgt[j]);
	}

	for (size_t i = 0; i < src.size(); i++) {
		// assuming planar transform
		vec2 p(xf[0] * src[i][0] + xf[4] * src[i][1] + xf[12], xf[1]
				* src[i][0] + xf[5] * src[i][1] + xf[13]);

		if (p[0] < mn[0] - maxsqdist)
			continue;
		if (p[0] > mx[0] + maxsqdist)
			continue;
		if (p[1] < mn[1] - maxsqdist)
			continue;
		if (p[1] > mx[1] + maxsqdist)
			continue;

		for (size_t j = 0; j < tgt.size(); j++) {
			if (src_used[i] && tgt_used[j])
				continue;

			if (dist2(p, tgt[j]) < maxsqdist) {
				//qDebug("Oldway: Hit found at (%d,%d)\t(%.3f < %.1f)", i, j, dist2(p, tgt[j]), maxsqdist);

				src_used[i] = 1;
				tgt_used[j] = 1;
				break;
			}
		}
	}
}

static void compute_contour_overlaps_stupid(const thera::Poly2 &src, const thera::Poly2 &tgt, const thera::XF &xf, std::vector<bool> &src_used, std::vector<bool> &tgt_used) {
	// anything marked as used will be preserved as used
	assert(src_used.size() == src.size());
	assert(tgt_used.size() == tgt.size());
	const float maxsqdist = 1;

	vec2 mn = tgt[0], mx = tgt[0];
	for (size_t j = 1; j < tgt.size(); j++) {
		mn = std::min(mn, tgt[j]);
		mx = std::max(mx, tgt[j]);
	}

	for (size_t i = 0; i < src.size(); i++) {
		// assuming planar transform
		vec2 p(xf[0] * src[i][0] + xf[4] * src[i][1] + xf[12], xf[1]
				* src[i][0] + xf[5] * src[i][1] + xf[13]);

		for (size_t j = 0; j < tgt.size(); j++) {
			if (src_used[i] && tgt_used[j])
				continue;


			if (p[0] < mn[0] - maxsqdist)
				continue;
			if (p[0] > mx[0] + maxsqdist)
				continue;
			if (p[1] < mn[1] - maxsqdist)
				continue;
			if (p[1] > mx[1] + maxsqdist)
				continue;

			if (dist2(p, tgt[j]) < maxsqdist) {
				src_used[i] = 1;
				tgt_used[j] = 1;
				break;
			}
		}
	}
}

QList<SQLFragmentConf> MatchConflictChecker::getConflicting() const {
	QElapsedTimer t;

	qDebug() << "Checking conflicts for" << mMaster.getTargetId() << "<->" << mMaster.getSourceId() << mMaster.index();

	t.start();

    const Fragment *targetFragment = Database::fragment(mMaster.mFragments[IFragmentConf::TARGET]);
    const Fragment *sourceFragment = Database::fragment(mMaster.mFragments[IFragmentConf::SOURCE]);

    CPoly2 targetContour = targetFragment->contour();
    CPoly2 sourceContour = sourceFragment->contour();

    targetContour.pin();
    sourceContour.pin();

    qDebug() << "Pinning and retrieving =" << t.restart() << "msec";

    //QBitArray targetUsed((*targetContour).size());
    //QBitArray sourceUsed((*sourceContour).size());

    QBitArray qbaTu((*targetContour).size());
	QBitArray qbaSu((*sourceContour).size());
	computeOverlap(*sourceContour, *targetContour, mMaster.mXF, qbaSu, qbaTu);

	for (int k = 0; k < 2000; ++k) {
		QBitArray targetUsed((*targetContour).size());
		QBitArray sourceUsed((*sourceContour).size());
		computeOverlap(*sourceContour, *targetContour, mMaster.mXF, sourceUsed, targetUsed);
	}
	qDebug() << "Found" << 0 << "overlaps in" << t.restart() << "msec";

	std::vector<bool> tgtu;
	tgtu.resize((*targetContour).size());

	std::vector<bool> srcu;
	srcu.resize((*sourceContour).size());

	compute_contour_overlaps(*sourceContour, *targetContour, mMaster.mXF, srcu, tgtu);

	for (int k = 0; k < 2000; ++k) {
	    std::vector<bool> tgt_used;
	    tgt_used.resize((*targetContour).size());

	    std::vector<bool> src_used;
	    src_used.resize((*sourceContour).size());

		//computeOverlap(*sourceContour, *targetContour, mMaster.mXF, sourceUsed, targetUsed);
		compute_contour_overlaps(*sourceContour, *targetContour, mMaster.mXF, src_used, tgt_used);
	}
	qDebug() << "std::vector: found" << 0 << "overlaps in" << t.elapsed() << "msec";

	for (int k = 0; k < 2000; ++k) {
		std::vector<bool> tgt_used;
		tgt_used.resize((*targetContour).size());

		std::vector<bool> src_used;
		src_used.resize((*sourceContour).size());

		//computeOverlap(*sourceContour, *targetContour, mMaster.mXF, sourceUsed, targetUsed);
		compute_contour_overlaps_stupid(*sourceContour, *targetContour, mMaster.mXF, src_used, tgt_used);
	}
	qDebug() << "std::vector stupid: found" << 0 << "overlaps in" << t.elapsed() << "msec";

	assert(qbaSu.size() == srcu.size());
	assert(qbaTu.size() == tgtu.size());
	for (int r = 0; r < qbaSu.size(); ++r) {
		if (qbaSu.testBit(r) != srcu[r]) {
			qDebug() << "SOURCE FAIL FAIL FAIL FAIL FAIL" << r << "(type = " << ((srcu[r]) ? "NEW MISSES)" : "OLD MISSES)");
			//assert(0 == 1);
		}
	}

	qDebug() << "The sources are already the same (or are they?)";

	for (int r = 0; r < qbaTu.size(); ++r) {
		if (qbaTu.testBit(r) != tgtu[r]) {
			qDebug() << "TARGET FAIL FAIL FAIL FAIL FAIL ON" << r << "(type = " << ((tgtu[r]) ? "NEW MISSES)" : "OLD MISSES)");
			//assert(0 == 1);
		}
	}

	qDebug() << "Results are the same!!!";

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
	int lastHit = 0;
	int secondToLastHit = 0;
	int hitCounter = 0;
	bool directionDetermined = false;

	// determine directionality of match
	register int i = 0, ii = source.size(), jj = target.size();
	for (; i < ii && !directionDetermined; ++i) {
		// assuming planar transform
		const vec2 p(
			xf[0] * source[i][0] + xf[4] * source[i][1] + xf[12],
			xf[1] * source[i][0] + xf[5] * source[i][1] + xf[13]
		);

		// early fail checks
		if (p[0] < mn[0] - maxsqdist) continue;
		if (p[0] > mx[0] + maxsqdist) continue;
		if (p[1] < mn[1] - maxsqdist) continue;
		if (p[1] > mx[1] + maxsqdist) continue;

		//qDebug() << "passed first continue tests";
		//int delta = sgn(lastHit - secondToLastHit);
		//qDebug("sgn(%d - %d) == %d", lastHit, secondToLastHit, delta);

		for (register int j = 0; j < jj; ++j) {
			if (sourceUsed.testBit(i) && targetUsed.testBit(j)) continue;

			if (dist2(p, target[j]) < maxsqdist) {
				++hitCounter;

				sourceUsed.setBit(i);
				targetUsed.setBit(j);

				secondToLastHit = lastHit;
				lastHit = j;

				if (hitCounter >= 2 && lastHit - secondToLastHit != 0) directionDetermined = true;

				//qDebug("Initialization hit found at (%d,%d)\t(%.3f < %.1f)", i, j, dist2(p, target[j]), maxsqdist);

				break;
			}
		}
	}

	//qDebug() << "passed first continue tests";
	const register int delta = sgn(lastHit - secondToLastHit);
	//qDebug("sgn(%d - %d) == %d", lastHit, secondToLastHit, delta);

	// the fast track
	//for (register int i = 2, ii = source.size(), jj = target.size(); i < ii; ++i) {
	for (; i < ii; ++i) {
		// assuming planar transform
		const vec2 p(
			xf[0] * source[i][0] + xf[4] * source[i][1] + xf[12],
			xf[1] * source[i][0] + xf[5] * source[i][1] + xf[13]
		);

		// early fail checks
		if (p[0] < mn[0] - maxsqdist) continue;
		if (p[0] > mx[0] + maxsqdist) continue;
		if (p[1] < mn[1] - maxsqdist) continue;
		if (p[1] > mx[1] + maxsqdist) continue;

		//for (register int j = 0; j < jj; ++j) {
		for (register int j = lastHit; j < jj && j >= 0; j += delta) {
			if (sourceUsed.testBit(i) && targetUsed.testBit(j)) continue;

			if (dist2(p, target[j]) < maxsqdist) {
				sourceUsed.setBit(i);
				targetUsed.setBit(j);

				lastHit = j;

				//qDebug("Fast track: Hit found at (%d,%d)\t(%.3f < %.1f)", i, j, dist2(p, target[j]), maxsqdist);
				break;
			}
			/*
			else if (j - lastHit) {

			}
			*/
		}
	}
}
