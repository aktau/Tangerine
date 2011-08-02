#ifndef MERGEITEM_H_
#define MERGEITEM_H_

#include <QString>
#include <QList>

#include "SQLDatabase.h"

#include "MergeActionTypes.h"
#include "MergeMapper.h"

class SimpleMergeAction;
class PreferUserAction;
class MergeAction;

class MergeItem {
	public:
		typedef enum {
			MATCHMERGEITEM
		} MergeItemType;

	public:
		MergeItem();
		virtual ~MergeItem();

		bool isResolved() const; // will return true if it has a valid action assigned
		bool isDone() const; // will return true if it has been correctly executed
		Merge::Action currentActionType() const;
		const MergeAction *currentAction() const;

		virtual MergeItemType type() const = 0;
		virtual QString typeString() const = 0;
		virtual QString message() const = 0;

		virtual QList<Merge::Action> acceptedActions() const { return QList<Merge::Action>(); }
		virtual bool acceptsAction(const MergeAction *action) const;

		// the default is NOOP, the accept() methods do NOT take ownership of the action
		// and do NOT keep a reference to them, it is safe to delete them after calling one of these
		// methods
		virtual void accept(const SimpleMergeAction *) { return; }
		virtual void accept(const PreferUserAction *) { return; }

		// TODO: create static QStrings and return by const reference
		//virtual QString getQuery() const { return QString(); }
		virtual bool execute(SQLDatabase *db, MergeMapper *mapper) = 0;

	protected:
		void store(const MergeAction *action);

		void setDone(bool done);

	protected:
		bool mDone;

		MergeAction *mAction;

		QString mQuery;
};

#endif /* MERGEITEM_H_ */
