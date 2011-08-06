#ifndef MATCHMODEL_H_
#define MATCHMODEL_H_

#include "IMatchModel.h"

#include <QSet>
#include <QList>
#include <QRegExp>

#include "SQLFragmentConf.h"
#include "InvalidFragmentConf.h"
#include "SQLDatabase.h"
#include "ModelParameters.h"

/**
 * The main implementation of a match model
 */
class MatchModel : public IMatchModel {
		Q_OBJECT

	public:
		// refreshInterval specifies how many milliseconds it takes for the MatchModel to ask the database whether anything changed
		// if 0, refreshing is disabled
		MatchModel(SQLDatabase *db, int refreshInterval = 0, QObject *parent = NULL);
		virtual ~MatchModel();

		void setDatabase(SQLDatabase *db);

	public:
		virtual void prefetchHint(int start, int end);
		virtual void preloadMatchData(bool preload, const QStringList& fields = QStringList()) ;

		virtual void setWindowSize(int size);
		virtual int getWindowSize() const;

		virtual bool isValidIndex(int index) const;
		virtual int size() const;
		virtual void sort(const QString& field = QString(), Qt::SortOrder order = Qt::AscendingOrder);
		virtual void filter(const QString& pattern = QString());
		virtual void genericFilter(const QString& key, const QString& filter);
		virtual void neighbours(int index, NeighbourMode mode = IMatchModel::ALL, bool keepParameters = false);

		virtual void initBatchModification();
		virtual void endBatchModification();

		virtual thera::IFragmentConf& get(int index);

		virtual void setParameters(const ModelParameters& parameters);
		virtual const ModelParameters& getParameters() const;

		virtual bool addField(const QString& name, double defaultValue);
		virtual bool addField(const QString& name, const QString& defaultValue);
		virtual bool addField(const QString& name, int defaultValue);
		virtual bool removeField(const QString& name);
		virtual QSet<QString> fieldList() const;
		virtual QString getFilter() const;

		virtual bool setDuplicates(QList<int> duplicates, int master, DuplicateMode mode = IMatchModel::ABSORB);
		virtual bool setMaster(int master);

	private:
		thera::SQLFragmentConf *getSQL(int index);

		bool requestWindow(int windowIndex);
		void requestRealSize();

		bool populateModel();

		// these do NOT emit signals
		void resetWindow();
		void resetSort();
		void resetFilter();

		// versions without signal emitting that return whether anything changed
		bool setSort(const QString& field, Qt::SortOrder order, ModelParameters& p);
		bool setNameFilter(const QString& pattern, ModelParameters& p);
		bool setGenericFilter(const QString& key, const QString& filter, ModelParameters& p); // adds, replaces or removes said key

		void neighbours(const thera::SQLFragmentConf& match, NeighbourMode mode = IMatchModel::ALL, bool keepParameters = false);

		void convertGroupToMaster(int groupMatchId, int masterMatchId);

	private slots:
		//void matchCountChanged();
		void databaseModified();

		void refresh();

	private:
		SQLDatabase *mDb;

		ModelParameters mPar;
		ModelParameters mDelayedPar;

		QList<thera::SQLFragmentConf> mMatches;
		thera::InvalidFragmentConf mInvalidFragmentConf;

		int mRealSize;
		int mWindowSize, mNextWindowOffset, mWindowOffset;
		int mWindowBegin, mWindowEnd;

		bool mDelayed, mDirty;

		bool mPreload;
		QStringList mPreloadFields;
};

#endif /* MATCHMODEL_H_ */
