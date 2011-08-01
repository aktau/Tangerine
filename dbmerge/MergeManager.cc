#include "MergeManager.h"

#include <QtGui>
#include <QtAlgorithms>

#include "main.h"

#include "ActionPickerDialog.h"

MergeManager::MergeManager(QWidget *parent, QSharedPointer<SQLDatabase> master) : QDialog(parent), mLeft(master), mCurrentPhase(-1) {
	mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

	// disable the OK button for now
	QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
	okButton->setEnabled(false);

	mBackwardButton = new QPushButton(QIcon(":/rcc/fatcow/32x32/resultset_previous.png"), QString());
	mBackwardButton->setEnabled(false);
	connect(mBackwardButton, SIGNAL(clicked()), this, SLOT(goBackward()));
	mButtonBox->addButton(mBackwardButton, QDialogButtonBox::ActionRole);

	mForwardButton = new QPushButton(QIcon(":/rcc/fatcow/32x32/resultset_next.png"), QString());
	mForwardButton->setEnabled(false);
	connect(mForwardButton, SIGNAL(clicked()), this, SLOT(goForward()));
	mButtonBox->addButton(mForwardButton, QDialogButtonBox::ActionRole);

	connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

	mItemList = new MergeTableWidget(0, 3);
	mItemList->setHorizontalHeaderLabels(QStringList() << tr("Type") << tr("Action") << tr("Message"));
	mItemList->setColumnWidth(0, 50);
	mItemList->setColumnWidth(1, 50);
	//mItemList->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	//mItemList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	mItemList->horizontalHeader()->setStretchLastSection(true);
	mItemList->verticalHeader()->setDefaultSectionSize(20);
	mItemList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	mItemList->setAlternatingRowColors(true);
	mItemList->setSelectionBehavior(QAbstractItemView::SelectRows);
	mItemList->setClickableColumn(1); // 0-based column indexing

	connect(mItemList, SIGNAL(itemDoubleClicked(QTableWidgetItem *)), this, SLOT(pickAction(QTableWidgetItem *)));

	// showing which databases are selected
	mMasterDbButton = new QPushButton(QIcon(":/rcc/fatcow/32x32/folder_explore.png"), QString());
	connect(mMasterDbButton, SIGNAL(clicked()), this, SLOT(pickDatabases()));

	mSlaveDbButton = new QPushButton(QIcon(":/rcc/fatcow/32x32/folder_explore.png"), QString());
	connect(mSlaveDbButton, SIGNAL(clicked()), this, SLOT(pickDatabases()));

	mMasterDbInfo = new QLineEdit(tr(""));
	mMasterDbInfo->setEnabled(false);

	mSlaveDbInfo = new QLineEdit(tr(""));
	mSlaveDbInfo->setEnabled(false);

	QGridLayout *formLayout = new QGridLayout;
	formLayout->addWidget(new QLabel("Master database:"), 0, 0);
	formLayout->addWidget(mMasterDbInfo, 0, 1);
	formLayout->addWidget(mMasterDbButton , 0, 2);
	formLayout->addWidget(new QLabel("Slave database:"), 1, 0);
	formLayout->addWidget(mSlaveDbInfo, 1, 1);
	formLayout->addWidget(mSlaveDbButton, 1, 2);

	mShowResolvedEntries = new QCheckBox(tr("&show resolved entries"));
	mShowResolvedEntries->setChecked(true);
	//connect(mShowResolvedEntries, SIGNAL(toggled()), this, SLOT(refresh()));
	connect(mShowResolvedEntries, SIGNAL(toggled(bool)), this, SLOT(refresh()));

	QHBoxLayout *bottomRowLayout = new QHBoxLayout;
	bottomRowLayout->addWidget(mShowResolvedEntries);
	bottomRowLayout->addWidget(mButtonBox);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addLayout(formLayout);
	layout->addWidget(mItemList);
	layout->addLayout(bottomRowLayout);

	resize(800, 380);
	setLayout(layout);

	setWindowTitle(tr("Merge databases"));

	updateDbInfo();
	updateProgressButtons();
}

MergeManager::~MergeManager() {
	qDeleteAll(mMergers);
	mMergers.clear();
}

void MergeManager::addMerger(Merger *merger) {
	mMergers << merger;

	updateProgressButtons();
}

void MergeManager::merge() {
	/*
	QList<HistoryRecord> leftHistory = left->getHistory("comment");

	foreach (const HistoryRecord& record, leftHistory) {
		qDebug() << record.userId << record.matchId << record.timestamp << record.value;
	}
	*/

	if (haveDatabases() && mCurrentPhase >= 0 && mCurrentPhase < mMergers.size()) {
		Merger *merger = mMergers.at(mCurrentPhase);

		merger->setMapper(&mMapper);
		merger->merge(mLeft.data(), mRight.data());

		//refresh();

		//addItemsToTable(merger->items());
	}
	else {
		qDebug() << "MergeManager::merge: was called but the manager state is not right" << haveDatabases() << "and current phase is" << mCurrentPhase << "of" << mMergers.size();
	}
}

void MergeManager::refresh() {
	mItemList->clearContents();

	if (!haveDatabases() || !(mCurrentPhase >= 0) || !(mCurrentPhase < mMergers.size())) return;

	const QList<MergeItem *>& items = mMergers.at(mCurrentPhase)->items();

	mItemList->setRowCount(items.size());

	int row = 0;
	foreach (const MergeItem *item, items) {
		if (item->isResolved() && !mShowResolvedEntries->isChecked()) {
			continue;
		}

		mItemList->setRow(row++, item);
	}

	mItemList->resizeColumnToContents(0);
	mItemList->resizeColumnToContents(1);
}

/*
void MergeManager::addItemsToTable(const QList<MergeItem *>& items) {
	qDebug() << "MergeManager::addItemsToTable: Going to add" << items.size() << "to table";

	mItemList->clearContents();

	mItemList->setRowCount(items.size());

	int row = 0;
	foreach (const MergeItem *item, items) {
		if (item->isResolved() && !mShowResolvedEntries->isChecked()) continue;

		mItemList->setRow(row++, item);
	}

	mItemList->resizeColumnToContents(0);
	mItemList->resizeColumnToContents(1);
	//mItemList->resizeRowsToContents();
}
*/

void MergeManager::pickDatabases() {
	bool pickOnlyOne = false;

	int i = 0;

	//QPushButton *button = qobject_cast<QPushButton *>(sender());

	if (sender() == mMasterDbButton) {
		i = 0;
		pickOnlyOne = true;
	}
	else if (sender() == mSlaveDbButton) {
		i = 1;
		pickOnlyOne = true;
	}
	else {
		if (!mLeft.isNull() && mLeft->isOpen()) {
			++i;
		}
	}

	QSettings settings;
	QString lastMatchDb = settings.value(SETTINGS_DB_LASTMATCHDB_KEY).toString();

	if (!lastMatchDb.isEmpty()) {
		QFileInfo fi(lastMatchDb);
		lastMatchDb = fi.dir().path();
	}

	while (i < 2) {
		QSharedPointer<SQLDatabase> &current = (i == 0) ? mLeft : mRight;

		QString fileName = QFileDialog::getSaveFileName(
			this,
			(i == 0) ? tr("Choose the master database") : tr("Choose the database to merge into the master database"),
			lastMatchDb,
			QString(),
			0,
			QFileDialog::DontConfirmOverwrite
		);

		if (!fileName.isEmpty()) {
			QString opening = tr("Opening db...");
			if (current == mLeft) mMasterDbInfo->setText(opening);
			else mSlaveDbInfo->setText(opening);

			current = SQLDatabase::getDb(fileName);

			if (!current->isOpen()) {
				QMessageBox::information(this, tr("Error while opening database"), tr("Couldn't open database, please try again or try another database"));
				--i;
			}
			else {
				++i;

				if (pickOnlyOne) break;
			}
		}
		else {
			break;
		}
	}

	updateProgressButtons();
	updateDbInfo();

	if (haveDatabases()) {
		// save the user from having to click on the button
		goForward();
	}
}

void MergeManager::updateDbInfo() {
	QStringList info;

	for (int i = 0; i < 2; ++i) {
		QSharedPointer<SQLDatabase> &current = (i == 0) ? mLeft : mRight;

		if (current.isNull()) {
			info << "No db chosen";
		}
		else if (!current->isOpen()) {
			info << "database not open, pick another one";
		}
		else {
			info << current->connectionName();
		}
	}

	assert(info.size() == 2);

	mMasterDbInfo->setText(info.at(0));
	mSlaveDbInfo->setText(info.at(1));

	mMasterDbButton->setEnabled(isBeginPhase());
	mSlaveDbButton->setEnabled(isBeginPhase());
}

void MergeManager::pickAction(QTableWidgetItem *item) {
	qDebug() << "One:" << item->column() << mItemList->clickableColumn();

	if (item->column() == mItemList->clickableColumn()) {
		int rowIndex = item->row();
		int lastPhase = mCurrentPhase;
		// show a dialog/messagebox/... where the user can choose an appropriate action

		qDebug() << "Two:" << rowIndex << "and phase =" << mCurrentPhase;

		if (lastPhase >= 0 && lastPhase < mMergers.size()) {
			Merger *merger =  mMergers.at(lastPhase);
			const QList<MergeItem *>& items = merger->items();

			qDebug() << "Three:" << merger << items.size();

			if (rowIndex >= 0 && rowIndex < items.size()) {
				MergeItem *item = items.at(rowIndex);

				MergeAction *currentActionCopy = item->currentAction()->clone();
				QList<MergeAction *> actionList = mActionFactory.createActions(item->acceptedActions(), currentActionCopy);

				{
					ActionPickerDialog actionPicker(this);
					actionPicker.setActions(actionList);
					actionPicker.setDefaultAction(currentActionCopy);

					if (actionPicker.exec() == QDialog::Accepted) {
						MergeAction *chosenAction = actionPicker.chosenAction();

						ActionApplyToItem mode = JUST_THIS;

						if (actionPicker.applyToSameType()) {
							mode = SAME_TYPE;
						}
						else if (actionPicker.applyToAccepting()) {
							mode = SAME_ACCEPT;
						}

						applyActionTo(chosenAction, item, mode);

						//chosenAction->visit(item);

						// refresh the list?
						refresh();
						//addItemsToTable(merger->items());
					}
				}

				qDeleteAll(actionList);
				actionList.clear();
			}
		}
	}
}

void MergeManager::applyActionTo(const MergeAction *action, MergeItem *mainItem, ActionApplyToItem mode) {
	assert(isProcessPhase());
	assert(mainItem->acceptsAction(action));

	const QList<MergeItem *>& items = mMergers.at(mCurrentPhase)->items();

	if (mode == JUST_THIS) {
		action->visit(mainItem);
	}
	else if (mode == SAME_TYPE) {
		foreach (MergeItem *item, items) {
			if (item->type() == mainItem->type()) {
				action->visit(item);
			}
		}
	}
	else if (mode == SAME_ACCEPT) {
		foreach (MergeItem *item, items) {
			if (item->acceptsAction(action)) {
				action->visit(item);
			}
		}
	}
	else {
		qDebug() << "MergeManager::applyActionTo: unknown mode";
	}
}

void MergeManager::goBackward() {
	--mCurrentPhase;
	assert(isValidPhase());

	refresh();
	updateProgressButtons();
}

void MergeManager::goForward() {
	++mCurrentPhase;
	assert(isValidPhase());

	if (!isEndPhase()) {
		merge();
	}

	refresh();
	updateProgressButtons();
}

void MergeManager::updateProgressButtons() {
	//qDebug() << canGoBack() << canAdvance() << haveDatabases() << mCurrentPhase << mMergers.size() - 1;

	mBackwardButton->setEnabled(canGoBack());
	mForwardButton->setEnabled(canAdvance());

	mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(isEndPhase());
}

inline bool MergeManager::isValidPhase() const {
	return (mCurrentPhase >= -1 && mCurrentPhase <= mMergers.size());
}

inline bool MergeManager::isProcessPhase() const {
	return (mCurrentPhase >= 0 && mCurrentPhase < mMergers.size());
}

inline bool MergeManager::isBeginPhase() const {
	return mCurrentPhase == -1;
}

inline bool MergeManager::isEndPhase() const {
	return mCurrentPhase == mMergers.size();
}

inline bool MergeManager::haveDatabases() const {
	return (!mLeft.isNull() && !mRight.isNull());
}

inline bool MergeManager::canGoBack() const {
	return haveDatabases() && (mCurrentPhase > 0);
}

inline bool MergeManager::canAdvance() const {
	return haveDatabases() && (mCurrentPhase < mMergers.size());
}

