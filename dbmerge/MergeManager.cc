#include "MergeManager.h"

#include <QtGui>
#include <QtAlgorithms>

MergeManager::MergeManager(QWidget *parent) : QDialog(parent) {
	mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
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
