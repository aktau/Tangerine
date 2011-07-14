#ifndef MATCHMODEL_H_
#define MATCHMODEL_H_

#include "IMatchModel.h"

#include <QSet>
#include <QList>
#include <QRegExp>

#include "SQLFragmentConf.h"

#include "SQLDatabase.h"

/**
 * The main implementation of a match model
 */
class MatchModel : public IMatchModel {
		Q_OBJECT

	public:
		MatchModel(SQLDatabase *db);
		virtual ~MatchModel();

	public:
		virtual void prefetchHint(int start, int end);

		virtual void setWindowSize(int size);
		virtual int getWindowSize() const;

		virtual bool isValidIndex(int index) const;
		virtual int size() const;
		virtual void sort(const QString& field = QString(), Qt::SortOrder order = Qt::AscendingOrder);
		virtual void filter(const QString& pattern = QString());
		virtual void genericFilter(const QString& key, const QString& filter);
		virtual void neighbours(int index, NeighbourMode mode = IMatchModel::ALL, bool keepParameters = false);
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
		thera::SQLFragmentConf& getSQL(int index);

		void requestWindow(int windowIndex);
		void requestRealSize();

		void populateModel();

		// these do NOT emit signals
		void resetWindow();
		void resetSort();
		void resetFilter();

		// versions without signal emitting that return whether anything changed
		bool setSort(const QString& field, Qt::SortOrder order);
		bool setNameFilter(const QString& pattern);
		bool setGenericFilter(const QString& key, const QString& filter); // adds, replaces or removes said key

		void convertGroupToMaster(int groupMatchId, int masterMatchId);

	private slots:
		//void matchCountChanged();
		void databaseModified();

	private:
		SQLDatabase *mDb;

		ModelParameters mPar;

		QList<thera::SQLFragmentConf> mMatches;

		int mRealSize;
		int mWindowSize, mWindowOffset;
		int mWindowBegin, mWindowEnd;
};

#endif /* MATCHMODEL_H_ */
