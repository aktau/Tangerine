#ifndef MATCHCONFLICTCHECKER_H_
#define MATCHCONFLICTCHECKER_H_

#include <QBitArray>

#include "SQLFragmentConf.h"

#include "XF.h"
#include "CPoly.h"

class MatchConflictChecker {
	public:
		MatchConflictChecker(thera::SQLFragmentConf master, QList<thera::SQLFragmentConf> list);

	public:
		QList<thera::SQLFragmentConf> getConflicting() const;
		QList<thera::SQLFragmentConf> getNonconflicting() const;

	private:
		QList<thera::SQLFragmentConf> filterList(bool conflicting = true) const;

		void computeOverlap(const thera::Poly2 &source, const thera::Poly2 &target, const thera::XF& xf, QBitArray& sourceUsed, QBitArray& targetUsed) const;
		bool conflicts(const QBitArray& a, const QBitArray& b, int threshold) const;

	private:
		thera::SQLFragmentConf mMaster;
		QList<thera::SQLFragmentConf> mList;

		QHash<int, thera::CPoly2> mContours;
};

#endif /* MATCHCONFLICTCHECKER_H_ */
