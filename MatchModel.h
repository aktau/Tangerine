#ifndef MATCHMODEL_H_
#define MATCHMODEL_H_

#include <QObject>
#include <QList>

#include "SQLFragmentConf.h"

class MatchModel : public QObject {
		Q_OBJECT

	public:
		MatchModel();
		virtual ~MatchModel();

		// we use this method in the beginning to avoid making the database connection from the model side
		// but we should, this is actually just dummy functionality!
		// TODO: dummy
		void setMatches(QList<thera::SQLFragmentConf> matches);
		void sortMatches(const QString& field, bool ascending = false);
		template <typename LessThan> void sortMatches(LessThan lessThan);

		int size() const;
		thera::SQLFragmentConf& get(int index);
		thera::SQLFragmentConf& last();

	signals:
		void modelChanged();
		void orderChanged();

	public:
		static MatchModel EMPTY;

	private:
		QList<thera::SQLFragmentConf> mMatches;

};

template <typename LessThan> inline void MatchModel::sortMatches(LessThan lessThan) {
	qSort(mMatches.begin(), mMatches.end(), lessThan);

	emit orderChanged();
}

inline int MatchModel::size() const {
	return mMatches.size();
}

inline thera::SQLFragmentConf& MatchModel::get(int index) {
	return mMatches[index];
}

inline thera::SQLFragmentConf& MatchModel::last() {
	return mMatches.last();
}

#endif /* MATCHMODEL_H_ */
