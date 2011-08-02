#ifndef MERGEMANAGER_H_
#define MERGEMANAGER_H_

#include <QDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSharedPointer>
#include <QLineEdit>
#include <QCheckBox>
#include <QList>

#include "SQLDatabase.h"

#include "MergeTableWidget.h"
#include "Merger.h"
#include "MergeMapper.h"
#include "ActionFactory.h"

class MergeManager : public QDialog {
		Q_OBJECT

public:
		typedef enum {
			JUST_THIS,
			SAME_TYPE_UNRESOLVED,
			SAME_ACCEPT_UNRESOLVED,
			SAME_TYPE_ALL,
			SAME_ACCEPT_ALL,
		} ActionApplyToItem;
	public:
		MergeManager(QWidget *parent = NULL, QSharedPointer<SQLDatabase> master = QSharedPointer<SQLDatabase>());
		~MergeManager();

	public:
		// takes ownership of the merger, do NOT delete yourself
		void addMerger(Merger *merger);

	protected:
		void merge();

		void applyActionTo(const MergeAction *action, MergeItem *item, ActionApplyToItem mode);

	private slots:
		void refresh(); // refresh the table widget (reload data according to current options/phase/etc

		void pickDatabases();
		void pickAction(QTableWidgetItem *item);

		void goBackward();
		void goForward();

		void updateAll();
		void updateProgressButtons();
		void updateDbInfo();

	private:
		Merger *currentMerger() const;
		QList<MergeItem *> getCurrentItems() const;

		bool isValidPhase() const;
		bool isProcessPhase() const;
		bool isBeginPhase() const;
		bool isEndPhase() const;
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

		QPushButton *mMasterDbButton;
		QPushButton *mSlaveDbButton;

		QLineEdit *mMasterDbInfo;
		QLineEdit *mSlaveDbInfo;

		QCheckBox *mShowResolvedEntries;

		// merge fields
		QSharedPointer<SQLDatabase> mLeft, mRight;
		QList<Merger *> mMergers;

		MergeMapper mMapper;
		ActionFactory mActionFactory;

		int mCurrentPhase;
};

#endif /* MERGEMANAGER_H_ */
