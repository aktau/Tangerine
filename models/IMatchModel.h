#ifndef IMATCHMODEL_H_
#define IMATCHMODEL_H_

#include <QObject>
#include <QDebug>
#include <QStringList>

#include "IFragmentConf.h"

/**
 * The pure abstract interface for all match models
 */
class IMatchModel : public QObject {
		Q_OBJECT

	public:
		enum Status { UNKNOWN, YES, MAYBE, NO, CONFLICT, NUM_STATUSES };
		static QStringList STATUS_STRINGS;

	public:
		IMatchModel() {}
		virtual ~IMatchModel() { }

	public:
		virtual bool isValidIndex(int index) const = 0;
		virtual int size() const = 0;
		virtual void sort(const QString& field = QString(), Qt::SortOrder order = Qt::AscendingOrder) = 0;
		virtual void filter(const QString& pattern = QString()) = 0;
		virtual void genericFilter(const QString& key, const QString& filter) = 0; // the syntax is the same as the SQL WHERE-clause syntax
		virtual thera::IFragmentConf& get(int index) = 0;

		virtual bool addField(const QString& name, double defaultValue) = 0;
		virtual bool addField(const QString& name, const QString& defaultValue) = 0;
		virtual bool addField(const QString& name, int defaultValue) = 0;
		virtual bool removeField(const QString& name) = 0;
		virtual QSet<QString> fieldList() const = 0;
		virtual QString getFilter() const = 0;

		virtual bool setDuplicates(QList<int> duplicates, int master) = 0;

	signals:
		void modelChanged();
		void orderChanged();
};

#endif /* IMATCHMODEL_H_ */
