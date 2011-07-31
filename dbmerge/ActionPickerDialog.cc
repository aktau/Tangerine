#include "ActionPickerDialog.h"

#include <QtGui>

ActionPickerDialog::ActionPickerDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f) {
	mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

	QGridLayout *grid = new QGridLayout;

	QGroupBox *groupBox = new QGroupBox(tr("All possible actions"));

	/*
	QRadioButton *radio1 = new QRadioButton(tr("&Radio button 1"));
	QRadioButton *radio2 = new QRadioButton(tr("R&adio button 2"));
	QRadioButton *radio3 = new QRadioButton(tr("Ra&dio button 3"));
	*/

	mActionBox = new QVBoxLayout;

	/*
	mActionBox->addWidget(radio1);
	mActionBox->addWidget(radio2);
	mActionBox->addWidget(radio3);
	*/
	mActionBox->addStretch(1);

	groupBox->setLayout(mActionBox);

	grid->addWidget(groupBox, 0, 0);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(groupBox);
	layout->addWidget(mButtonBox);

	connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

	setLayout(layout);
	setWindowTitle(tr("Pick the action you wish to take"));
}

ActionPickerDialog::~ActionPickerDialog() {

}

void ActionPickerDialog::setActions(const QList<MergeAction *>& actions) {
	mActions = actions;

	foreach (MergeAction *action, mActions) {
		QRadioButton *radio = new QRadioButton(action->description());
		mActionBox->addWidget(radio);
	}
}

MergeAction *ActionPickerDialog::chosenAction() const {

}
