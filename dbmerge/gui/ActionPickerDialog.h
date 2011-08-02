#ifndef ACTIONPICKERDIALOG_H_
#define ACTIONPICKERDIALOG_H_

#include <QDialog>
#include <QGroupBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QLayout>
#include <QMap>

#include "MergeAction.h"

class ActionPickerDialog : public QDialog {
		Q_OBJECT

	public:
		ActionPickerDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
		virtual ~ActionPickerDialog();

		// does NOT take ownership of the actions, don't delete them while the dialog is active
		void setActions(const QList<MergeAction *>& actions);
		void setDefaultAction(const MergeAction *action);
		MergeAction *chosenAction() const;

		bool applyToSameTypeUnresolved() const;
		bool applyToSameType() const;
		bool applyToAcceptingUnresolved() const;
		bool applyToAccepting() const;

	private:
		QDialogButtonBox *mButtonBox;

		typedef QPair<QRadioButton *, MergeAction *> ButtonActionPair;
		QList<ButtonActionPair> mActionMap;
		//QMap<QRadioButton *, MergeAction *> mActionMap;
		//QList<MergeAction *> mActions;

		QVBoxLayout *mActionBox;
		QGroupBox *mGroupBox;

		QCheckBox *mAllSameType, *mAllSameTypeUnresolved;
		QCheckBox *mAllAccepting, *mAllAcceptingUnresolved;
};

#endif /* ACTIONPICKERDIALOG_H_ */
