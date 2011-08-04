#include "ActionFactory.h"

#include <QDebug>

ActionFactory::ActionFactory() {

}

ActionFactory::~ActionFactory() {
	qDeleteAll(mConstActions);
	mConstActions.clear();
}

const MergeAction *ActionFactory::getConstAction(Merge::Action type) {
	ConstActionIterator i = mConstActions.constFind(type);

	if (i != mConstActions.constEnd()) {
		return i.value();
	}
	else {
		MergeAction *action = createAction(type);
		mConstActions.insert(type, action);

		return action;
	}
}

QList<const MergeAction *> ActionFactory::getConstActions(const QList<Merge::Action>& typeList) {
	QList<const MergeAction *> actionList;

	foreach (Merge::Action type, typeList) {
		actionList << getConstAction(type);
	}

	return actionList;
}

MergeAction *ActionFactory::createAction(Merge::Action type) {
	/*
  		NONE,
		CHOOSE_SLAVE,
		PREFER_USER,
		MOST_RECENT,
		DONT_MERGE,
		ASSIGN_NEW_ID
	 */
	switch (type) {
		//case Merge::CHOOSE_MASTER: return new ChooseMasterAction;
		case Merge::CHOOSE_SLAVE: return new ChooseSlaveAction;
		case Merge::CHOOSE_HISTORY: return new ChooseHistoryAction;
		case Merge::PREFER_USER: return new PreferUserAction;
		case Merge::MOST_RECENT: return new MostRecentAction;
		case Merge::ASSIGN_NEW_ID: return new AssignIdAction;
		case Merge::DONT_MERGE: return new DontMergeAction;

		default:
			qDebug() << "ActionFactory::createAction: MATCHTYPE NOT RECOGNIZED" << type;
			return new NoAction;
	}
}

QList<MergeAction *> ActionFactory::createActions(const QList<Merge::Action>& typeList, MergeAction *insertAction) {
	QList<MergeAction *> actionList;

	foreach (Merge::Action type, typeList) {
		if (insertAction && insertAction->type() == type) {
			actionList << insertAction;
		}
		else {
			actionList << createAction(type);
		}
	}

	return actionList;
}
