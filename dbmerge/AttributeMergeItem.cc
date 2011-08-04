#include "AttributeMergeItem.h"

#include <QtGui>

#define MERGE_DEBUG 2

struct HistoryLessThan {
    bool operator()(const HistoryRecord& lhs, const HistoryRecord& rhs) const {
        return lhs.timestamp < rhs.timestamp;
    }
};

AttributeMergeItem::AttributeMergeItem(int matchId, const QList<HistoryRecord>& masterHistory, const QList<HistoryRecord>& slaveHistory, const QString& attributeName)
	: mMatchId(matchId), mMasterHistory(masterHistory), mSlaveHistory(slaveHistory), mMergeIn(false), mAttributeName(attributeName) { }

void AttributeMergeItem::accept(const SimpleMergeAction *action) {
	MergeAction *newAction = NULL;

	qDebug() << "AttributeMergeItem::accept: accepting" << action;

	switch (action->type()) {
		case Merge::CHOOSE_SLAVE: {
			chooseSlave();
		} break;

		case Merge::MOST_RECENT: {
			chooseMostRecent();
		} break;

		case Merge::CHOOSE_HISTORY: {
			newAction = chooseHistory();
		} break;

		case Merge::DONT_MERGE:
			mMessage = QObject::tr("User chose not to merge");
			break;

		default:
			qDebug() << "MatchMergeItem::accept: received unacceptable action";
			return;
	}

	store((newAction) ? newAction : action);

	delete newAction;
}

bool AttributeMergeItem::execute(SQLDatabase *db, MergeMapper *mapper) {
	bool success = true;

	if (currentActionType() == Merge::ASSIGN_NEW_ID) {

	}

	//mapper->addMapping(MergeMapper::MATCH_ID, mOldId, mNewId);

	setDone(true);

	return true;
}

MergeAction *AttributeMergeItem::chooseMostRecent() {
	qDebug() << "Choosing most recent!!!!";

	MergeAction *newAction = NULL;

	if (newAction = decideForInvalidHistories()) {
		return newAction;
	}

	const HistoryRecord& leftMostRecent = mMasterHistory.last();
	const HistoryRecord& rightMostRecent = mSlaveHistory.last();

	HistoryLessThan hLess;
	if (hLess(leftMostRecent, rightMostRecent)) {
		// pick right
		mMessage = "Slave was edited more recently than master";
		newAction = new ChooseSlaveAction;
	}
	else {
		mMessage = "Master was edited more recently than slave";
		newAction = new DontMergeAction;
	}

	return newAction;
}

MergeAction *AttributeMergeItem::chooseSlave() {
	qDebug() << "Choosing slave!!!!";

	MergeAction *newAction = NULL;

	if (newAction = decideForInvalidHistories()) {
		return newAction;
	}

	mMessage = "Chose slave";
	newAction = new ChooseSlaveAction;

	return newAction;
}

MergeAction *AttributeMergeItem::chooseHistory() {
	MergeAction *newAction = NULL;

	if (newAction = decideForInvalidHistories()) {
		return newAction;
	}

	const HistoryRecord& leftMostRecent = mMasterHistory.last();
	const HistoryRecord& rightMostRecent = mSlaveHistory.last();

	if (leftMostRecent == rightMostRecent) {
#ifdef MERGE_DEBUG
		qDebug() << "RESULT: Both were equal, not doing anything: " << leftMostRecent.toString();
		qDebug() << "----------------------------------";
#endif

		mMessage = "No action required, both histories are the same";
		return new DontMergeAction;
	}

#if MERGE_DEBUG >= 2
	qDebug() << "Master history";
	int i = 0;
	foreach (const HistoryRecord& r, mMasterHistory) {
		qDebug() << ++i << "-" << r.toString();
	}

	qDebug() << "Slave history";
	i = 0;
	foreach (const HistoryRecord& r, mSlaveHistory) {
		qDebug() << ++i << "-" << r.toString();
	}
#endif

	HistoryLessThan hLess;
	QList<HistoryRecord>::const_iterator found = qBinaryFind(mSlaveHistory.begin(), mSlaveHistory.end(), leftMostRecent, hLess);

	// look for shared history, conflict, resolve
	if (found != mSlaveHistory.end()) {
		if (leftMostRecent == *found) {
			qDebug() << "RESULT: Found master history inside of slave: supposed to merge!";
			mMessage = QString("Slave is more current than master, merging");
		}
		else {
			qDebug() << "RESULT: Freak accident #1: exactly the same timestamp but not the same, what to do?:" << leftMostRecent.toString() << "vs" << found->toString();
			newAction = new NoAction;
			mMessage = QString("Couldn't verify history agreement, manual action required");

			//while (++found != rightHistory.end()) { ... }
		}
	}
	else {
		qDebug() << "RESULT: Did NOT find master history inside of slave";

		found = qBinaryFind(mMasterHistory.begin(), mMasterHistory.end(), rightMostRecent, hLess);
		if (found != mMasterHistory.end()) {
			if (rightMostRecent == *found) {
				qDebug() << "RESULT: Found last slave update inside of master history: do not merge, master is more recent!";
				mMessage = QString("Master is more current than slave, not merging");
			}
			else {
				qDebug() << "RESULT: Freak accident #2: exactly the same timestamp but not the same, what to do?:" << rightMostRecent.toString() << "vs" << found->toString();
				newAction = new NoAction;
				mMessage = QString("Couldn't verify history agreement, manual action required");
			}
		}
		else {
			qDebug() << "RESULT: Histories diverged or were never the same in the first place: conflict resolution magic here";
			mMessage = QString("Histories diverged or were never the same in the first place, manual action required");
			newAction = new NoAction;
		}
	}

	qDebug() << "----------------------------------";

	return newAction;
}

MergeAction *AttributeMergeItem::decideForInvalidHistories() {
	if (mSlaveHistory.isEmpty()) {
		// pick master no matter what

		mMessage = QString("Slave history wasn't available, picking master");
		return new DontMergeAction;
	}

	if (mMasterHistory.isEmpty()) {
		// pick slave now (because slave hist is not empty)

		mMessage = QString("Master history wasn't available, picking slave");
		return new ChooseSlaveAction;
	}

	return NULL;
}

QWidget *AttributeMergeItem::informationWidget() const {
	QTableWidget *masterTable = createTableWidget(mMasterHistory);
	QTableWidget *slaveTable = createTableWidget(mSlaveHistory);

	QLabel *title = new QLabel(QString("Showing data for attribute '<b>%1</b>' on match <b>%2<b>").arg(mAttributeName).arg(mMatchId));
	title->setTextFormat(Qt::RichText);

	//QHBoxLayout *layout = new QHBoxLayout;
	QGridLayout *layout = new QGridLayout;
	layout->addWidget(title, 0, 0);
	layout->addWidget(new QLabel("Master table:"), 1, 0);
	layout->addWidget(masterTable, 2, 0);
	layout->addWidget(new QLabel("Slave table:"), 1, 1);
	layout->addWidget(slaveTable, 2, 1);

	QWidget *widget = new QWidget();
	widget->setLayout(layout);

	return widget;
}

QTableWidget *AttributeMergeItem::createTableWidget(const QList<HistoryRecord>& history) const {
	QTableWidget *table = new QTableWidget(0, 3);

	table->setHorizontalHeaderLabels(QStringList() << QObject::tr("Time") << QObject::tr("User") << QObject::tr("Value"));
	//table->setColumnWidth(0, 50);
	//table->setColumnWidth(1, 50);
	//mItemList->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	//mItemList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	table->horizontalHeader()->setStretchLastSection(true);
	table->verticalHeader()->setDefaultSectionSize(20);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setAlternatingRowColors(true);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);

	/*
	QTableWidgetItem *actionDescription = new QTableWidgetItem(action->description());
	actionDescription->setForeground(!item->isResolved() ? QBrush(Qt::red) : QBrush(Qt::green));
	actionDescription->setData(Qt::UserRole, qVariantFromValue((void *) item));
	*/

	table->setRowCount(history.size());

	int row = 0;
	foreach (const HistoryRecord& record, history) {
		qDebug() << "Filling up" << record.timestamp.toString() << record.userId << record.matchId << record.value.toString();

		QTableWidgetItem *time = new QTableWidgetItem(record.timestamp.toString("hh:mm dd/MM/yy"));
		time->setForeground(QBrush(Qt::white));
		time->setBackgroundColor(Qt::black);

		table->setItem(row, 0, time);
		table->setItem(row, 1, new QTableWidgetItem(QString::number(record.userId)));
		//table->setItem(row, 2, new QTableWidgetItem(QString::number(record.matchId)));
		table->setItem(row, 2, new QTableWidgetItem(record.value.toString()));

		++row;
	}

	table->resizeColumnsToContents();
	//mItemList->resizeColumnToContents(1);

	return table;
}
