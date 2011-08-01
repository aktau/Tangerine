#include "MergeItem.h"

#include "MergeAction.h"

MergeItem::MergeItem() : mAction(new NoAction)  { }
MergeItem::~MergeItem() { delete mAction; }

bool MergeItem::isResolved() const { return mAction; }
Merge::Action MergeItem::currentActionType() const { return mAction ? mAction->type() : Merge::NONE; }
const MergeAction *MergeItem::currentAction() const { return mAction; }

void MergeItem::store(const MergeAction *action) {
	delete mAction;
	mAction = action->clone();
}
