#ifndef IMATCHMODEL_H_
#define IMATCHMODEL_H_

#include <QObject>
#include <QDebug>
#include <QStringList>

#include "IFragmentConf.h"

class IMatchModel : public QObject {
		Q_OBJECT

	public:
		enum Status { UNKNOWN, YES, MAYBE, NO, CONFLICT, NUM_STATUSES };
		static QStringList STATUS_STRINGS;

	public:
		IMatchModel() {}
		virtual ~IMatchModel() { }

	public:
		virtual int size() const = 0;
		virtual void sort(const QString& field, Qt::SortOrder order = Qt::AscendingOrder) = 0;
		virtual void filter(const QString& pattern) = 0;
		virtual thera::IFragmentConf& get(int index) = 0;

		virtual QSet<QString> fieldList() const = 0;
		virtual QString getFilter() const = 0;

	signals:
		void modelChanged();
		void orderChanged();
};

#endif /* IMATCHMODEL_H_ */
