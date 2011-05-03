#ifndef EMPTYMATCHMODEL_H_
#define EMPTYMATCHMODEL_H_

#include "IMatchModel.h"

#include <QDebug>

#include "InvalidFragmentConf.h"

class EmptyMatchModel : public IMatchModel {
		Q_OBJECT

	public:
		EmptyMatchModel() { }
		virtual ~EmptyMatchModel() { }

	public:
		int size() const { return 0; }
		void sort(const QString& field, Qt::SortOrder order = Qt::AscendingOrder) { qDebug() << "EmptyMatchModel::sort: Attempted to sort, field:" << field << "| order:" << order; }
		void filter(const QString& pattern) { qDebug() << "EmptyMatchModel::filter: Attempted to filter, pattern:" << pattern; }
		thera::IFragmentConf& get(int index) {
			qDebug() << "EmptyMatchModel::get: Attempted to get fragmentconf:" << index;

			return mFc;
		}

		QSet<QString> fieldList() const {  qDebug() << "EmptyMatchModel::fieldList: Attempted to get field list"; return QSet<QString>(); }
		QString getFilter() const {  qDebug() << "EmptyMatchModel::getFilter: Attempted to get filter"; return QString(); }

	public:
		// create one default instance that everyone can conveniently use
		static EmptyMatchModel EMPTY;

	private:
		thera::InvalidFragmentConf mFc;
};

#endif /* EMPTYMATCHMODEL_H_ */
