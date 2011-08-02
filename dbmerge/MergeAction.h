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

		virtual MergeAction *clone() const = 0;
		virtual Merge::Action type() const = 0;
		virtual QString description() const = 0;
		virtual QWidget *createWidget() const { return NULL; };

		virtual void visit(MergeItem *item) const = 0;
};

class NoAction : public MergeAction {
	MergeAction *clone() const { return new NoAction(*this); }
	Merge::Action type() const { return Merge::NONE; }
	QString description() const { return "No action"; }

	void visit(MergeItem *) const { }
};

// a simple action with no arguments, these can all be handled similarly because they only "functionally" differ in their type
class SimpleMergeAction : public MergeAction {
};

class ChooseMasterAction : public SimpleMergeAction {
	public:
		MergeAction *clone() const { return new ChooseMasterAction(*this); }
		Merge::Action type() const { return Merge::CHOOSE_MASTER; }
		QString description() const { return "Choose master"; }

		void visit(MergeItem *item) const { item->accept(this); }
};

class DontMergeAction : public SimpleMergeAction {
	public:
		MergeAction *clone() const { return new DontMergeAction(*this); }
		Merge::Action type() const { return Merge::DONT_MERGE; }
		QString description() const { return "Don't merge"; }

		void visit(MergeItem *item) const { item->accept(this); }
};

class AssignIdAction : public SimpleMergeAction {
	public:
		AssignIdAction() : mNewId(-1) { }
		AssignIdAction(int newId) : mNewId(newId) { }

		MergeAction *clone() const { return new AssignIdAction(*this); }
		Merge::Action type() const { return Merge::ASSIGN_NEW_ID; }
		QString description() const {
			return (mNewId != -1) ? QString("Assign id: %1").arg(mNewId) : QString("Assign a new random id");
		}

		void visit(MergeItem *item) const { item->accept(this); }

		int assignId() const { return mNewId; }

	private:
		int mNewId;
};

class PreferUserAction : public MergeAction {
	public:
		MergeAction *clone() const { return new PreferUserAction(*this); }
		Merge::Action type() const { return Merge::PREFER_USER; }
		QString description() const { return "Prefer user"; }

		void visit(MergeItem *item) const { item->accept(this); }
};


#endif /* MERGEACTION_H_ */
