#ifndef IMATCHMODEL_H_
#define IMATCHMODEL_H_

#include <QObject>
#include <QDebug>
#include <QStringList>

#include "IFragmentConf.h"
//#include "ModelParameters.h"

class ModelParameters;

/**
 * The pure abstract interface for all match models
 *
 * VERY IMPORTANT: calling nearly any method on the model will invalidate the references obtained via get(). Luckily calling these functions will yield
 * the modelChanged() signals et cetera, so you always know when this happens. However, take care not to get references, call methods and then keep using those
 * references in the same method.
 *
 * NOTE: do NOT ignore the modelChanged() and orderChanged() signals, they singal an invalidating of the references
 */
class IMatchModel : public QObject {
		Q_OBJECT

	public:
		typedef enum {
			ABSORB,
			ORPHAN,

			DUPLICATE_MODES
		} DuplicateMode;

		typedef enum {
			ALL,
			CONFLICTING,
			NONCONFLICTING,

			NEIGHBOUR_MODES
		} NeighbourMode;

		enum Status { UNKNOWN, YES, MAYBE, NO, CONFLICT, NUM_STATUSES };
		static QStringList STATUS_STRINGS;

	public:
		IMatchModel(QObject *parent) : QObject(parent) {}
		virtual ~IMatchModel() { }

	public:
		// performance switch, doesn't affect functionality
		virtual void prefetchHint(int start, int end) = 0; // some model types may completely ignore this

		// performance switch, doesn't affect functionality
		// some model types may completely ignore this, if no fields to preload are specified the preload list won't be updated (i.e.: the list from the last call will be used)
		// temporarily disable preloading like this: preloadMatchData(false); ... get matches; preloadMatchData(true); <--- the same preload fields will be reinstated
		virtual void preloadMatchData(bool preload, const QStringList& fields = QStringList()) = 0;

		virtual bool isValidIndex(int index) const = 0;
		virtual int size() const = 0;
		virtual void sort(const QString& field = QString(), Qt::SortOrder order = Qt::AscendingOrder) = 0;
		virtual void filter(const QString& pattern = QString()) = 0;
		virtual void genericFilter(const QString& key, const QString& filter) = 0; // the syntax is the same as the SQL WHERE-clause syntax
		virtual void neighbours(int index, NeighbourMode mode = IMatchModel::ALL, bool keepParameters = false) = 0;

		// these methods will delay the changing (and notification thereof) of the model
		// an example:
		// 	model.initBatchModification();
		//	model.filter(...)
		//	model.sort(...)
		//  model.endBBatchModification();
		// This promotes efficient usage of resources
		virtual void initBatchModification() = 0;
		virtual void endBatchModification() = 0;

		// Arguably the most important method in the class. However, it has a problem
		// It returns by reference, and this reference will not be good forever. In fact
		// every new filter, sort, whatever will invalidate the reference. If this model
		// has more than one client, this could be disastrous and lead to segmentation
		// faults all the time. What we should be doing is returning by value. However,
		// IFragmentConf is an abstract class, the actual value class would be SQLFragmentConf.
		// This would impede the smooth integration of the new SQL-layer with any past and future
		// layers however... which would be sad. Currently I have no idea how to solve this.
		//
		// TODO: Fix this! (first proposal: (Qt) smart pointers)... well references only change after modelChanged() and orderChanged()
		virtual thera::IFragmentConf& get(int index) = 0;

		// used to get and restore the state of the model completely
		virtual void setParameters(const ModelParameters& parameters) = 0;
		virtual const ModelParameters& getParameters() const = 0;

		virtual bool addField(const QString& name, double defaultValue) = 0;
		virtual bool addField(const QString& name, const QString& defaultValue) = 0;
		virtual bool addField(const QString& name, int defaultValue) = 0;
		virtual bool removeField(const QString& name) = 0;
		virtual QSet<QString> fieldList() const = 0;
		virtual QString getFilter() const = 0;

		virtual bool setDuplicates(QList<int> duplicates, int master, DuplicateMode mode = IMatchModel::ABSORB) = 0;
		virtual bool setMaster(int master) = 0;
		virtual bool resetDuplicates(QList<int> duplicates) = 0;

	signals:
		void modelRefreshed(); // doesn't change the references, but signals that the metadata has been updated. All references are still valid, you can choose to ignore this
		void modelChanged(); // the entire model could have changed, all old references are no longer valid
		void orderChanged(); // the amount of matches hasn't changed, nor their values, but the order has changed, all references are no longer valid
};

#endif /* IMATCHMODEL_H_ */
