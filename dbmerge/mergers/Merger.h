#ifndef MERGER_H_
#define MERGER_H_

#include <QObject>
#include <QList>

#include "SQLDatabase.h"
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
		virtual void execute(SQLDatabase *left, MergeMapper *mapper) = 0;
		virtual const QList<MergeItem *>& items();
		virtual void clear();

		virtual bool isResolved() const; // all items have an action assigned to them
		virtual bool isDone() const; // all items have been executed

	protected:
		QList<MergeItem *> mItems;

		MergeMapper *mMapper;
};

#endif /* MERGER_H_ */
