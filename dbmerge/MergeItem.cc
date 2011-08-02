#include "MergeItem.h"

#include "MergeAction.h"

MergeItem::MergeItem() : mDone(false), mAction(new NoAction) { }
MergeItem::~MergeItem() { delete mAction; }

bool MergeItem::acceptsAction(const MergeAction *action) const {
	return (action) ? acceptedActions().contains(action->type()) : false;
}

bool MergeItem::isResolved() const { return mAction->type() != Merge::NONE; }
bool MergeItem::isDone() const { return mDone; }
Merge::Action MergeItem::currentActionType() const { return mAction ? mAction->type() : Merge::NONE; }
const MergeAction *MergeItem::currentAction() const { return mAction; }

void MergeItem::store(const MergeAction *action) {
	delete mAction;
	mAction = action->clone();
}

void MergeItem::setDone(bool done) {
	mDone = done;
}
