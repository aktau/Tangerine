#ifndef MERGER_H_
#define MERGER_H_

#include <QObject>
#include <QList>

#include "SQLDatabase.h"
#include "MergeConflict.h"

class Merger : public QObject {
		Q_OBJECT

	public:
		Merger();
		virtual ~Merger();

	public:
		virtual void merge(SQLDatabase *left, SQLDatabase *right) = 0;
		virtual QList<MergeConflict>& conflicts();
		virtual const QStringList queries() const;
		virtual void clear();

	protected:
		QStringList mQueries;
		QList<MergeConflict> mConflicts;
};

#endif /* MERGER_H_ */
