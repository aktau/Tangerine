#ifndef ACTIONFACTORY_H_
#define ACTIONFACTORY_H_

#include <QMap>

#include "MergeAction.h"
#include "MergeActionTypes.h"

class ActionFactory {
	public:
		ActionFactory();
		virtual ~ActionFactory();

		// ownership is NOT transferred to the caller, you can use this when for example you're only interested
		// in the message of a particular action, it's very fast and doesn't create any superfluous objects
		const MergeAction *getConstAction(Merge::Action type);
		QList<const MergeAction *> getConstActions(const QList<Merge::Action>& typeList);

		// ownership is transferred to the caller
		MergeAction *createAction(Merge::Action type);
		QList<MergeAction *> createActions(const QList<Merge::Action>& typeList, MergeAction *insertAction = NULL); // the insert action is inserted in the place where a "default" action would otherwise be inserted, this is for convenience

	protected:
		typedef QMap<Merge::Action, const MergeAction *> ConstActionMap;
		typedef ConstActionMap::const_iterator ConstActionIterator;
		QMap<Merge::Action, const MergeAction *> mConstActions;
};

#endif /* ACTIONFACTORY_H_ */
