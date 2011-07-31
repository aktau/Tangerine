#include "MergeManager.h"

#include <QtGui>
#include <QtAlgorithms>

#include "main.h"

#include "ActionPickerDialog.h"

MergeManager::MergeManager(QWidget *parent, QSharedPointer<SQLDatabase> master) : QDialog(parent), mLeft(master), mCurrentPhase(0) {
	mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

	// disable the OK button for now
	QPushButton *okButton = mButtonBox->button(QDialogButtonBox::Ok);
	okButton->setEnabled(false);

	QPushButton *moreButton = new QPushButton(tr("&Prepare merge"));
	connect(moreButton, SIGNAL(clicked()), this, SLOT(pickDatabases()));
	mButtonBox->addButton(moreButton, QDialogButtonBox::ActionRole);

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

	mItemList = new MergeTableWidget(1, 3);
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

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(mItemList);
	layout->addWidget(mButtonBox);

	resize(800, 380);
	setLayout(layout);

	setWindowTitle(tr("Merge databases"));
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

		addItemsToTable(merger->items());
	}
	else {
		qDebug() << "MergeManager::merge: was called but the manager state is not right" << haveDatabases() << "and current phase is" << mCurrentPhase << "of" << mMergers.size();
	}
}

void MergeManager::addItemsToTable(const QList<MergeItem *>& items) {
	qDebug() << "MergeManager::addItemsToTable: Going to add" << items.size() << "to table";

	mItemList->setRowCount(items.size());

	int row = 0;
	foreach (const MergeItem *item, items) {
		const MergeAction *action = mActionFactory.getConstAction(item->getCurrentAction());

		mItemList->setItem(row, 0, new QTableWidgetItem("Merge match"));

		QTableWidgetItem *actionDescription = new QTableWidgetItem(action->description());
		actionDescription->setForeground((action->type() == Merge::NONE) ? QBrush(Qt::red) : QBrush(Qt::green));
		mItemList->setItem(row, 1, actionDescription);

		mItemList->setItem(row, 2, new QTableWidgetItem(item->getMessage()));

		++row;
	}

	mItemList->resizeColumnToContents(0);
	mItemList->resizeColumnToContents(1);
	//mItemList->resizeRowsToContents();
}

void MergeManager::pickDatabases() {
	QSettings settings;
	QString lastMatchDb = settings.value(SETTINGS_DB_LASTMATCHDB_KEY).toString();

	if (!lastMatchDb.isEmpty()) {
		QFileInfo fi(lastMatchDb);
		lastMatchDb = fi.dir().path();
	}

	//QSharedPointer<SQLDatabase> left, right;

	int i = 0;

	if (!mLeft.isNull() && mLeft->isOpen()) {
		//left = mLeft;
		++i;
	}

	bool success = true;

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
			current = SQLDatabase::getDb(fileName);

			if (!current->isOpen()) {
				QMessageBox::information(this, tr("Error while opening database"), tr("Couldn't open database, please try again or try another database"));
				--i;
			}
			else {
				++i;
			}
		}
		else {
			success = false;
			break;
		}
	}

	/*
	if (success) {
		merge();
	}
	*/

	updateProgressButtons();
}

void MergeManager::pickAction(QTableWidgetItem *item) {
	qDebug() << "One:" << item->column() << mItemList->clickableColumn();

	if (item->column() == mItemList->clickableColumn()) {
		int rowIndex = item->row();
		int lastPhase = mCurrentPhase - 1;
		// show a dialog/messagebox/... where the user can choose an appropriate action

		qDebug() << "Two:" << rowIndex << "and phase =" << mCurrentPhase;

		if (lastPhase >= 0 && lastPhase < mMergers.size()) {
			Merger *merger =  mMergers.at(lastPhase);
			const QList<MergeItem *>& items = merger->items();

			qDebug() << "Three:" << merger << items.size();

			if (rowIndex >= 0 && rowIndex < items.size()) {
				MergeItem *item = items.at(rowIndex);

				QList<MergeAction *> actionList = mActionFactory.createActions(item->acceptedActions());

				{
					ActionPickerDialog actionPicker(this);
					actionPicker.setActions(actionList);
					actionPicker.exec();
				}

				qDeleteAll(actionList);
				actionList.clear();
			}
		}
	}
}

void MergeManager::goBackward() {
	// TODO: how to go back?

	--mCurrentPhase;

	assert(isValidPhase());

	updateProgressButtons();
}

void MergeManager::goForward() {
	merge();

	++mCurrentPhase;

	assert(isValidPhase());

	updateProgressButtons();
}

void MergeManager::updateProgressButtons() {
	qDebug() << canGoBack() << canAdvance() << haveDatabases() << mCurrentPhase << mMergers.size() - 1;

	mBackwardButton->setEnabled(canGoBack());
	mForwardButton->setEnabled(canAdvance());

	mButtonBox->button(QDialogButtonBox::Ok)->setEnabled(mCurrentPhase == mMergers.size());
}

inline bool MergeManager::isValidPhase() const {
	return (mCurrentPhase >= 0 && mCurrentPhase <= mMergers.size());
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

