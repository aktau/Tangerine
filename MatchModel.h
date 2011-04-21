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

		int size() const;
		thera::SQLFragmentConf& getMatch(int index);
		thera::SQLFragmentConf& last();

	signals:
		void modelChanged();

	public:
		static MatchModel EMPTY;

	private:
		QList<thera::SQLFragmentConf> mMatches;

};

inline int MatchModel::size() const {
	return mMatches.size();
}

inline thera::SQLFragmentConf& MatchModel::getMatch(int index) {
	return mMatches[index];
}

inline thera::SQLFragmentConf& MatchModel::last() {
	return mMatches.last();
}

#endif /* MATCHMODEL_H_ */
