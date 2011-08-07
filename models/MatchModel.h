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
		virtual bool resetDuplicates(QList<int> duplicates);

	private:
		thera::SQLFragmentConf *getSQL(int index);

		const QList<thera::SQLFragmentConf> fetchCurrentMatches();
		void autoAdjustRefreshTimer();

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

		void refresh(bool forceReloadOnConflict = false);

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

		QTimer *mRefreshTimer;
		int mBaseRefreshInterval;

		qint64 mLastQueryMsec;
};

// more likely inlining
inline const QList<thera::SQLFragmentConf> MatchModel::fetchCurrentMatches() {
	QElapsedTimer timer;
	timer.start();

	QList<thera::SQLFragmentConf> list = (mPreload) ?
		mDb->getPreloadedMatches(mPreloadFields, mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize) :
		mDb->getMatches(mPar.sortField, mPar.sortOrder, mPar.filter, mWindowBegin, mWindowSize);

	mLastQueryMsec = timer.elapsed();

	// it's possibly this method gets called not by refresh(), in which case we'll also want to restart the timer, it makes no
	// sense to refresh when we just fetched, also perhaps the interval needs some heuristic setting
	autoAdjustRefreshTimer();

	return list;
}

inline void MatchModel::autoAdjustRefreshTimer() {
	if (!mRefreshTimer) return;

	// start or restart
	mRefreshTimer->start();

	// if the database call took longer than 100 ms we're possibly having
	// 		slow internet
	// 		a slow query
	//		database server overloaded
	// --> the good thing to do is to throttle the amount of refreshes
	// TODO: provide an option to disable throttling
	if (mLastQueryMsec < 100) {
		if (mRefreshTimer->interval() > mBaseRefreshInterval) {
			// at some point in time the interval was set higher than the original interval because the db fetch was slow, it's fast again
			// so let's return to the default interval
			mRefreshTimer->setInterval(mBaseRefreshInterval);

			qDebug() << "MatchModel::autoAdjustRefreshTimer: last query took < 100 msec, putting refresh timer back to normal = " << mBaseRefreshInterval << "msec";
		}
	}
	else if (mLastQueryMsec < 500) {
		// the db fetch was pretty slow (more than 100 msec might be noticeable and possibly a drain on external DB resources)
		// but we're going to give it the benefit of the doubt and raise the refresh limit until it reaches once every 5 minutes
		const int newInterval = qMin(1000 * 60 * 5, mRefreshTimer->interval() * 2); // 5 minutes max

		qDebug() << "MatchModel::autoAdjustRefreshTimer: (last query took" << mLastQueryMsec << "msec, raised the refresh interval from"
			<< (float(mRefreshTimer->interval()) / 1000.f) << "to" << (float(newInterval) / 1000.f)
			<< "seconds because of slow database fetching (conserve resources)";

		mRefreshTimer->setInterval(newInterval);
	}
	else {
		// the query took more than half a second, that's too slow

		qDebug() << "MatchModel::autoAdjustRefreshTimer: (last query took" << mLastQueryMsec << "msec, stopped refreshing until a cheaper query is made";

		// put the interval to something ridiculously high because the fetching time was way too long
		mRefreshTimer->setInterval(100000000); // that's right, a 100 million seconds
		mRefreshTimer->stop(); // we can't stop it indefinitely because focusing the TangerineApplication could restart it, it doesn't matter with such an interval though
	}
}

#endif /* MATCHMODEL_H_ */
