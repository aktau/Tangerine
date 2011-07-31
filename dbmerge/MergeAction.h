#ifndef MERGEACTION_H_
#define MERGEACTION_H_

#include <QString>
#include <QWidget>

#include "MergeItem.h"
#include "MergeActionTypes.h"

class MergeAction {
	public:
		MergeAction() { }
		virtual ~MergeAction() { }

		virtual Merge::Action type() const = 0;
		virtual QString description() const = 0;
		virtual QWidget *createWidget() const { return NULL; };

		virtual void visit(MergeItem *item) const = 0;
};

class NoAction : public MergeAction {
	Merge::Action type() const { return Merge::NONE; }
	QString description() const { return "No action"; }

	void visit(MergeItem *) const { }
};

// a simple action with no arguments, these can all be handled similarly because they only "functionally" differ in their type
class SimpleMergeAction : public MergeAction {
};

class ChooseMasterAction : public SimpleMergeAction {
	public:
		Merge::Action type() const { return Merge::CHOOSE_MASTER; }
		QString description() const { return "Choose master"; }

		void visit(MergeItem *item) const { item->accept(this); }
};

class AssignIdAction : public SimpleMergeAction {
	public:
		Merge::Action type() const { return Merge::ASSIGN_NEW_ID; }
		QString description() const { return "Assign a new (random) ID"; }

		void visit(MergeItem *item) const { item->accept(this); }
};

class PreferUserAction : public MergeAction {
	public:
		Merge::Action type() const { return Merge::PREFER_USER; }
		QString description() const { return "Prefer user"; }

		void visit(MergeItem *item) const { item->accept(this); }
};


#endif /* MERGEACTION_H_ */
