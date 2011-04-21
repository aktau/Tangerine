#include "MatchModel.h"

using namespace thera;

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
