#include "ActionPickerDialog.h"

#include <QtGui>

ActionPickerDialog::ActionPickerDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f), mActionBox(NULL) {
	mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

	// TODO: display info about the item (left choice, right choice)

	// all actions
	mGroupBox = new QGroupBox(tr("All possible actions"));

	//mActionBox->addStretch(1);

	// options
	QGroupBox *groupBox = new QGroupBox(tr("Options"));

	mAllSameTypeUnresolved = new QCheckBox(tr("Apply action to all unresolved items of the same type"));
	mAllAcceptingUnresolved = new QCheckBox(tr("Apply action to all unresolved items that accept it"));
	mAllSameType = new QCheckBox(tr("Apply action to all resolved items as well"));
	mAllSameType->setEnabled(false);
	mAllAccepting = new QCheckBox(tr("Apply action to all resolved items as well"));
	mAllAccepting->setEnabled(false);

	QVBoxLayout *vbox = new QVBoxLayout;

	vbox->addWidget(mAllSameTypeUnresolved);
	{
		QHBoxLayout *hbl = new QHBoxLayout;
		hbl->addWidget(new QLabel("Optional:"));
		hbl->addWidget(mAllSameType);
		vbox->addLayout(hbl);
	}
	connect(mAllSameTypeUnresolved, SIGNAL(toggled(bool)),mAllSameType, SLOT(setEnabled(bool)));

	vbox->addWidget(mAllAcceptingUnresolved);
	{
		QHBoxLayout *hbl = new QHBoxLayout;
		hbl->addWidget(new QLabel("Optional:"));
		hbl->addWidget(mAllAccepting);
		vbox->addLayout(hbl);
	}
	connect(mAllAcceptingUnresolved, SIGNAL(toggled(bool)), mAllAccepting, SLOT(setEnabled(bool)));

	//vbox->addWidget(mAllSameType);
	//vbox->addWidget(mAllAccepting);

	vbox->addStretch(1);
	groupBox->setLayout(vbox);

	QVBoxLayout *layout = new QVBoxLayout;
	{
		layout->addWidget(mGroupBox);
		layout->addWidget(groupBox);
		layout->addWidget(mButtonBox);
	}
	setLayout(layout);

	setWindowTitle(tr("Pick the action you wish to take"));
}

ActionPickerDialog::~ActionPickerDialog() {

}

void ActionPickerDialog::setActions(const QList<MergeAction *>& actions) {
	//qDebug() << "ActionPickerDialog::setActions:";
	//qDebug() << actions;

	delete mActionBox;
	mActionBox = new QVBoxLayout;

	foreach (MergeAction *action, actions) {
		QRadioButton *radio = new QRadioButton(action->description());
		mActionBox->addWidget(radio);

		mActionMap << ButtonActionPair(radio, action);
	}

	// make sure at least one is checked
	mActionMap.first().first->setChecked(true);

	mGroupBox->setLayout(mActionBox);

	//qDebug() << "ActionPickerDialog::setActions: done";
}

void ActionPickerDialog::setDefaultAction(const MergeAction *action) {
	foreach (ButtonActionPair pair, mActionMap) {
		if (pair.second == action) {
			pair.first->setChecked(true);
			return;
		}
	}

	qDebug() << "ActionPickerDialog::setDefaultAction: action wasn't encountered among the already instantiated actions";
}

MergeAction *ActionPickerDialog::chosenAction() const {
	foreach (ButtonActionPair pair, mActionMap) {
		if (pair.first->isChecked()) {
			return pair.second;
		}
	}

	qDebug() << "ActionPickerDialog::setDefaultAction: no check action was found! Returning NULL (which will probably cause problems)";

	return NULL;
}

bool ActionPickerDialog::applyToSameTypeUnresolved() const {
	return mAllSameTypeUnresolved->isChecked();
}

bool ActionPickerDialog::applyToAcceptingUnresolved() const {
	return mAllAcceptingUnresolved->isChecked();
}

bool ActionPickerDialog::applyToSameType() const {
	return mAllSameTypeUnresolved->isChecked() && mAllSameType->isChecked();
}

bool ActionPickerDialog::applyToAccepting() const {
	return mAllAcceptingUnresolved->isChecked() && mAllAccepting->isChecked();
}
