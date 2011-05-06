#ifndef MATCHMODEL_H_
#define MATCHMODEL_H_

#include "IMatchModel.h"

#include <QSet>
#include <QList>
#include <QRegExp>

#include "SQLFragmentConf.h"

#include "SQLDatabase.h"

class MatchModel : public IMatchModel {
		Q_OBJECT

	public:
		MatchModel(SQLDatabase *db);
		virtual ~MatchModel();

	public:
		virtual int size() const;
		virtual void sort(const QString& field = QString(), Qt::SortOrder order = Qt::AscendingOrder);
		virtual void filter(const QString& pattern = QString());
		virtual thera::IFragmentConf& get(int index);

		virtual bool addField(const QString& name, double defaultValue);
		virtual bool addField(const QString& name, const QString& defaultValue);
		virtual bool addField(const QString& name, int defaultValue);
		virtual bool removeField(const QString& name);
		virtual QSet<QString> fieldList() const;
		virtual QString getFilter() const;

	private:
		void populateModel();

	private slots:
		void databaseModified();

	private:
		QList<thera::SQLFragmentConf> mMatches;

		QString mFilter;
		QString mSortField;
		Qt::SortOrder mSortOrder;

		SQLDatabase *mDb;

};

#endif /* MATCHMODEL_H_ */
