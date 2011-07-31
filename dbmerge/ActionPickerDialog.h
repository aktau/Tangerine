#ifndef ACTIONPICKERDIALOG_H_
#define ACTIONPICKERDIALOG_H_

#include <QDialog>
#include <QDialogButtonBox>
#include <QLayout>

#include "MergeAction.h"

class ActionPickerDialog : public QDialog {
		Q_OBJECT

	public:
		ActionPickerDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
		virtual ~ActionPickerDialog();

		// does not take ownership of the actions, don't delete them while the dialog is active
		void setActions(const QList<MergeAction *>& actions);
		MergeAction *chosenAction() const;

	private:
		QDialogButtonBox *mButtonBox;

		QList<MergeAction *> mActions;

		QVBoxLayout *mActionBox;
};

#endif /* ACTIONPICKERDIALOG_H_ */
