#include "MatchModel.h"

#include <QtAlgorithms>
#include <QDebug>
#include <QElapsedTimer>

using namespace thera;

struct MatchCompare {
	const QString field;
	const bool ascending;

	MatchCompare(QString field_, bool ascending_) : field(field_), ascending(ascending_) {}
	inline bool operator ()(const thera::IFragmentConf &fc1, const thera::IFragmentConf &fc2) {
		const double d1 = fc1.getDouble(field);
		const double d2 = fc2.getDouble(field);

		return ascending ? d1 < d2 : d1 > d2;
	}
};

MatchModel MatchModel::EMPTY;

MatchModel::MatchModel() : QObject() {
	// TODO Auto-generated constructor stub

}

MatchModel::~MatchModel() {
	// TODO Auto-generated destructor stub
}

void MatchModel::setMatches(QList<SQLFragmentConf> matches) {
	mMatches = matches;

	emit modelChanged();
}

void MatchModel::sortMatches(const QString& field, bool ascending) {
	QElapsedTimer timer;
	timer.start();

	MatchCompare mc(field, ascending);
	qSort(mMatches.begin(), mMatches.end(), mc);

	qDebug() << "MatchModel::sortMatches: Sorting took" << timer.elapsed() << "milliseconds";

	emit orderChanged();
}
