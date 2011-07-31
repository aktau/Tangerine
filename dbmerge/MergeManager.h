#ifndef MERGEMANAGER_H_
#define MERGEMANAGER_H_

#include <QDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSharedPointer>
#include <QList>

#include "SQLDatabase.h"

#include "MergeTableWidget.h"
#include "Merger.h"
#include "MergeMapper.h"
#include "ActionFactory.h"

class MergeManager : public QDialog {
		Q_OBJECT

	public:
		MergeManager(QWidget *parent = NULL, QSharedPointer<SQLDatabase> master = QSharedPointer<SQLDatabase>());
		~MergeManager();

	public:
		// takes ownership of the merger, do NOT delete yourself
		void addMerger(Merger *merger);

	protected:
		void merge();

		void addItemsToTable(const QList<MergeItem *>& items);

	private slots:
		void pickDatabases();
		void pickAction(QTableWidgetItem *item);

		void goBackward();
		void goForward();

		void updateProgressButtons();

	private:
		bool isValidPhase() const;
		bool canGoBack() const;
		bool canAdvance() const;
		bool haveDatabases() const;

	private:
		// widgets
		//QTableWidget *mItemList;
		MergeTableWidget *mItemList;
		QDialogButtonBox *mButtonBox;

		QPushButton *mBackwardButton;
		QPushButton *mForwardButton;

		// merge fields
		QSharedPointer<SQLDatabase> mLeft, mRight;
		QList<Merger *> mMergers;

		MergeMapper mMapper;
		ActionFactory mActionFactory;

		int mCurrentPhase;
};

#endif /* MERGEMANAGER_H_ */
