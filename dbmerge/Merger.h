#ifndef MERGER_H_
#define MERGER_H_

#include <QObject>
#include <QList>

#include "SQLDatabase.h"
#include "MergeConflict.h"
#include "MergeMapper.h"
#include "MergeItem.h"

class Merger : public QObject {
		Q_OBJECT

	public:
		Merger();
		virtual ~Merger();

	public:
		virtual void setMapper(MergeMapper *mapper);

		virtual void merge(SQLDatabase *left, SQLDatabase *right) = 0;
		virtual const QList<MergeItem *>& items();
		//virtual const QList<MergeConflict>& conflicts();
		virtual const QStringList queries() const;
		virtual void clear();

	protected:
		QStringList mQueries;
		//QList<MergeConflict> mConflicts;
		QList<MergeItem *> mItems;

		MergeMapper *mMapper;
};

#endif /* MERGER_H_ */
