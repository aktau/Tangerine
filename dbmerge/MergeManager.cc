#include "MergeManager.h"

#include <QtGui>
#include <QtAlgorithms>

#include "main.h"

MergeManager::MergeManager(QWidget *parent, QSharedPointer<SQLDatabase> master) : QDialog(parent), mMaster(master) {
	mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

	QPushButton *moreButton = new QPushButton(tr("&Merge"));
	connect(moreButton, SIGNAL(clicked()), this, SLOT(pickDatabases()));
	mButtonBox->addButton(moreButton, QDialogButtonBox::ActionRole);

	connect(mButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(mButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

	mItemList = new QTableWidget(0, 3);
	mItemList->setHorizontalHeaderLabels(QStringList() << tr("Type") << tr("Action") << tr("Message"));
	mItemList->setColumnWidth(0, 50);
	mItemList->setColumnWidth(1, 50);
	//mItemList->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	//mItemList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	mItemList->horizontalHeader()->setStretchLastSection(true);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(mItemList);
	layout->addWidget(mButtonBox);

	resize(640, 380);
	setLayout(layout);

	setWindowTitle(tr("Merge databases"));
}

MergeManager::~MergeManager() {
	qDeleteAll(mMergers);
	mMergers.clear();
}

void MergeManager::addMerger(Merger *merger) {
	mMergers << merger;
}

void MergeManager::merge(QSharedPointer<SQLDatabase> left, QSharedPointer<SQLDatabase> right) {
	qDebug() << "MergeManager::merge";

	QList<HistoryRecord> leftHistory = left->getHistory("comment");

	qDebug() << "MergeManager::merge: 2";

	foreach (const HistoryRecord& record, leftHistory) {
		qDebug() << record.userId << record.matchId << record.timestamp << record.value;
	}

	foreach (Merger *merger, mMergers) {
		merger->merge(left.data(), right.data());
	}
}

void MergeManager::pickDatabases() {
	QSettings settings;
	QString lastMatchDb = settings.value(SETTINGS_DB_LASTMATCHDB_KEY).toString();

	if (!lastMatchDb.isEmpty()) {
		QFileInfo fi(lastMatchDb);
		lastMatchDb = fi.dir().path();
	}

	QSharedPointer<SQLDatabase> left, right;

	int i = 0;

	if (!mMaster.isNull() && mMaster->isOpen()) {
		left = mMaster;
		++i;
	}

	bool success = true;

	while (i < 2) {
		QSharedPointer<SQLDatabase> &current = (i == 0) ? left : right;

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

	if (success) {
		merge(left, right);
	}
}