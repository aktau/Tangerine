#ifndef MERGEITEM_H_
#define MERGEITEM_H_

#include <QString>
#include <QList>

#include "MergeActionTypes.h"

class SimpleMergeAction;
class PreferUserAction;

class MergeItem {
	public:
		MergeItem();
		virtual ~MergeItem();

		inline bool isResolved() const { return mResolved; }

		virtual QString getMessage() const = 0;

		virtual QList<Merge::Action> acceptedActions() const { return QList<Merge::Action>(); }

		// the default is NOOP
		virtual void accept(const SimpleMergeAction *) { return; }
		virtual void accept(const PreferUserAction *) { return; }

		// TODO: create static QStrings and return by const reference
		virtual QString getQuery() const { return QString(); }

	protected:
		// this means that whatever is extracted from query can be taken to be "correct" (even if it is empty, which means a NOOP was necessary)
		bool mResolved;

		QString mQuery;
};

#endif /* MERGEITEM_H_ */
