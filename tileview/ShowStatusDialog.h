#ifndef SHOWSTATUSDIALOG_H_
#define SHOWSTATUSDIALOG_H_

#include <QDialog>

#include <QList>
#include <QPair>
#include <QCheckBox>

class ShowStatusDialog : public QDialog {
	Q_OBJECT

	public:
		typedef QPair<QString, bool> StringBoolPair;

	public:
		ShowStatusDialog(QWidget *parent = NULL, const QList< StringBoolPair >& statuses = QList< StringBoolPair >());
		virtual ~ShowStatusDialog();

	public:
		QList< StringBoolPair > getStatuses() const;

	private:
		QList<QCheckBox *> mCheckboxes;
};

#endif /* SHOWSTATUSDIALOG_H_ */
