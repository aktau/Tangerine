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
		virtual void setWindowSize(int size);

		virtual bool isValidIndex(int index) const;
		virtual int size() const;
		virtual void sort(const QString& field = QString(), Qt::SortOrder order = Qt::AscendingOrder);
		virtual void filter(const QString& pattern = QString());
		virtual void genericFilter(const QString& key, const QString& filter);
		virtual thera::IFragmentConf& get(int index);

		virtual bool addField(const QString& name, double defaultValue);
		virtual bool addField(const QString& name, const QString& defaultValue);
		virtual bool addField(const QString& name, int defaultValue);
		virtual bool removeField(const QString& name);
		virtual QSet<QString> fieldList() const;
		virtual QString getFilter() const;

		virtual bool setDuplicates(QList<int> duplicates, int master, DuplicateMode mode = IMatchModel::ABSORB);
		virtual bool setMaster(int master);

	private:
		void requestWindow(int windowIndex);
		void requestRealSize();

		void populateModel();

		// these do NOT emit signals
		void resetWindow();
		void resetSort();
		void resetFilter();

		void convertGroupToMaster(int groupMatchId, int masterMatchId);

	private slots:
		//void matchCountChanged();
		void databaseModified();

	private:
		QList<thera::SQLFragmentConf> mMatches;

		QString mNameFilter;
		QString mSortField;
		Qt::SortOrder mSortOrder;

		SQLDatabase *mDb;
		SQLFilter mFilter;

		int mRealSize;
		int mWindowSize;
		int mWindowBegin, mWindowEnd;
};

#endif /* MATCHMODEL_H_ */
