#include "ActionPickerDialog.h"

#include <QtGui>

ActionPickerDialog::ActionPickerDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f), mActionBox(NULL) {
	mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

	//QGridLayout *grid = new QGridLayout;

	mGroupBox = new QGroupBox(tr("All possible actions"));

	//mActionBox = new QVBoxLayout;

	//mActionBox->addStretch(1);

	//groupBox->setLayout(mActionBox);

	//grid->addWidget(mGroupBox, 0, 0);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(mGroupBox);
	layout->addWidget(mButtonBox);

	connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

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
