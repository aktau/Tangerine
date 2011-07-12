#ifndef MODELPARAMETERS_H_
#define MODELPARAMETERS_H_

#include <QString>

#include "SQLFilter.h"

struct ModelParameters {
	// filtering
	SQLFilter filter;
	// the filter on the names of the fragments is considered special because it's used so much, it will be converted to
	// a subfilter of SQLFilter in the model. Ergo, it's for convenience
	QString matchNameFilter;

	// sorting
	QString sortField;
	Qt::SortOrder sortOrder;

	ModelParameters(SQLDatabase *db)
		: filter(db), matchNameFilter(QString()), sortField(QString()), sortOrder(Qt::AscendingOrder) {}

	ModelParameters(SQLFilter _filter = SQLFilter(), QString _matchNameFilter = QString(), QString _sortField = QString(), Qt::SortOrder _sortOrder = Qt::AscendingOrder)
		: filter(_filter), matchNameFilter(_matchNameFilter), sortField(_sortField), sortOrder(_sortOrder) {}

	bool operator==(const ModelParameters& other) const {
		return sortOrder == other.sortOrder &&
				sortField == other.sortField &&
				matchNameFilter == other.matchNameFilter &&
				filter == other.filter;
	}
	bool operator!=(const ModelParameters& other) const {
		return !(*this == other);
	}
};

#endif /* MODELPARAMETERS_H_ */