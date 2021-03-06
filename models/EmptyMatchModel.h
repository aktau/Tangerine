#ifndef EMPTYMATCHMODEL_H_
#define EMPTYMATCHMODEL_H_

#include "IMatchModel.h"

#include <QDebug>

#include "InvalidFragmentConf.h"
#include "ModelParameters.h"

/**
 * Represents an empty match model to which nothing can be added, good for initializing classes that use
 * models but haven't received a real one yet
 */
class EmptyMatchModel : public IMatchModel {
		Q_OBJECT

	public:
		EmptyMatchModel(QObject *parent = NULL) : IMatchModel(parent) { }
		virtual ~EmptyMatchModel() { }

	public:
		void prefetchHint(int, int) { qDebug() << "EmptyMatchModel::prefetchHint"; }
		void preloadMatchData(bool, const QStringList&) { qDebug() << "EmptyMatchModel::preloadMatchData"; }

		bool isValidIndex(int index) const { qDebug() << "EmptyMatchModel::isValidIndex: tried to check if index" << index << "was valid"; return false; }
		int size() const { return 0; }
		void sort(const QString& field = QString(), Qt::SortOrder order = Qt::AscendingOrder) { qDebug() << "EmptyMatchModel::sort: Attempted to sort, field:" << field << "| order:" << order; }
		void filter(const QString& pattern = QString()) { qDebug() << "EmptyMatchModel::filter: Attempted to filter, pattern:" << pattern; }
		void genericFilter(const QString& key, const QString& filter) { qDebug() << "EmptyMatchModel::genericFilter: Attempted to filter generically, key:" << key << "| filter" << filter; }
		void neighbours(int, NeighbourMode, bool) { qDebug() << "EmptyMatchModel::neighbours"; }

		void initBatchModification() { qDebug() << "EmptyMatchModel::initBatchModification"; }
		void endBatchModification() { qDebug() << "EmptyMatchModel::endBatchModification"; }

		thera::IFragmentConf& get(int index) {
			qDebug() << "EmptyMatchModel::get: Attempted to get fragmentconf:" << index;

			return mFc;
		}

		void setParameters(const ModelParameters&) { qDebug() << "EmptyMatchModel::setParameters"; }
		const ModelParameters& getParameters() const { qDebug() << "EmptyMatchModel::getParameters"; return mModelParameters; }

		bool addField(const QString& name, double defaultValue) { qDebug() << "EmptyMatchModel::addField:" << name << "||" << defaultValue; return false; }
		bool addField(const QString& name, const QString& defaultValue) { qDebug() << "EmptyMatchModel::addField:" << name << "||" << defaultValue; return false; }
		bool addField(const QString& name, int defaultValue) { qDebug() << "EmptyMatchModel::addField:" << name << "||" << defaultValue; return false; }
		bool removeField(const QString& name) { qDebug() << "EmptyMatchModel::removeField:" << name; return false; }
		QSet<QString> fieldList() const {  qDebug() << "EmptyMatchModel::fieldList: Attempted to get field list"; return QSet<QString>(); }
		QString getFilter() const {  qDebug() << "EmptyMatchModel::getFilter: Attempted to get filter"; return QString(); }

		bool setDuplicates(QList<int>, int, DuplicateMode) { qDebug() << "EmptyMatchModel::setDuplicates"; return false; }
		bool setMaster(int) { qDebug() << "EmptyMatchModel::setMaster"; return false; };
		bool resetDuplicates(QList<int>) { return false; }

	public:
		// create one default instance that everyone can conveniently use
		static EmptyMatchModel EMPTY;

	private:
		thera::InvalidFragmentConf mFc;

		const ModelParameters mModelParameters;
};

#endif /* EMPTYMATCHMODEL_H_ */
