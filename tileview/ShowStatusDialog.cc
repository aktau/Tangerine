#include "ShowStatusDialog.h"

#include <QFormLayout>
#include <QDialogButtonBox>

ShowStatusDialog::ShowStatusDialog(QWidget *parent, const QList< StringBoolPair >& statuses) : QDialog(parent) {
	setWindowTitle("Statuses to display");

	QFormLayout *formLayout = new QFormLayout;

	foreach (const StringBoolPair& status, statuses) {
		mCheckboxes << new QCheckBox(this);
		mCheckboxes.last()->setText(status.first);
		mCheckboxes.last()->setChecked(status.second);

		formLayout->addRow(mCheckboxes.last());
	}

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	formLayout->addWidget(buttonBox);

	setLayout(formLayout);
}

ShowStatusDialog::~ShowStatusDialog() {

}

QList< QPair<QString, bool> > ShowStatusDialog::getStatuses() const {
	QList< StringBoolPair > list;

	foreach (const QCheckBox *checkbox, mCheckboxes) {
		list << StringBoolPair(checkbox->text(), checkbox->isChecked());
	}

	return list;
}
