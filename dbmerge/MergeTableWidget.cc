#include "MergeTableWidget.h"

#include <QMouseEvent>
#include <QDebug>

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

void MergeTableWidget::mouseMoveEvent(QMouseEvent *event) {
    int column = columnAt(event->x());

    if (column == -1 || column != mClickableColumn) {
    	setCursor(Qt::ArrowCursor);
    }
    else {
    	setCursor(Qt::PointingHandCursor);
    }
}
