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
		CHOOSE_MASTER,
		CHOOSE_SLAVE,
		PREFER_USER,
		MOST_RECENT,
		DONT_MERGE,
		ASSIGN_NEW_ID
	 */
	switch (type) {
		case Merge::CHOOSE_MASTER: return new ChooseMasterAction;
		case Merge::CHOOSE_SLAVE: return new NoAction;
		case Merge::PREFER_USER: return new NoAction;
		case Merge::MOST_RECENT: return new NoAction;
		case Merge::ASSIGN_NEW_ID: return new AssignIdAction;
		case Merge::DONT_MERGE: return new NoAction;

		default:
			qDebug() << "ActionFactory::createAction: MATCHTYPE NOT RECOGNIZED" << type;
			return new NoAction;
	}
}

QList<MergeAction *> ActionFactory::createActions(const QList<Merge::Action>& typeList) {
	QList<MergeAction *> actionList;

	foreach (Merge::Action type, typeList) {
		actionList << createAction(type);
	}

	return actionList;
}
