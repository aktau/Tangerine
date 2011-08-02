#include "MergeTableWidget.h"

#include <QMouseEvent>
#include <QDebug>

#include "MergeItem.h"
#include "MergeAction.h"

MergeTableWidget::MergeTableWidget(QWidget *parent) : QTableWidget(parent), mClickableColumn(-1) { init(); }

MergeTableWidget::MergeTableWidget(int rows, int columns, QWidget *parent) : QTableWidget(rows, columns, parent), mClickableColumn(-1) { init(); }

MergeTableWidget::~MergeTableWidget() { }

inline void MergeTableWidget::init() {
	setMouseTracking(true);
}

void MergeTableWidget::setClickableColumn(int column) {
	mClickableColumn = column;
}

int MergeTableWidget::clickableColumn() const {
	return mClickableColumn;
}

void MergeTableWidget::setRow(int row, const MergeItem *item) {
	const MergeAction *action = item->currentAction();

	QTableWidgetItem *actionDescription = new QTableWidgetItem(action->description());
	actionDescription->setForeground(!item->isResolved() ? QBrush(Qt::red) : QBrush(Qt::green));
	actionDescription->setData(Qt::UserRole, qVariantFromValue((void *) item));

	setItem(row, 0, new QTableWidgetItem(item->typeString()));
	setItem(row, 1, actionDescription);
	setItem(row, 2, new QTableWidgetItem(item->message()));
}

void MergeTableWidget::mouseMoveEvent(QMouseEvent *event) {
    int column = columnAt(event->x());

    if (column == -1 || column != mClickableColumn) {
    	setCursor(Qt::ArrowCursor);
    }
    else {
    	setCursor(Qt::PointingHandCursor);
    }
}
