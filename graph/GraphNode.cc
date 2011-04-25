#include "GraphNode.h"

#include <QDebug>
#include <QPen>
#include <QGraphicsSceneMouseEvent>

GraphNode::GraphNode(const QRectF& rect, QGraphicsItem *parent) : QGraphicsEllipseItem(centerRect(rect), parent) {
	setPos(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
}

GraphNode::GraphNode(qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent) : QGraphicsEllipseItem(x, y, width, height, parent) {
}

GraphNode::~GraphNode() {
}

void GraphNode::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	qDebug() << "CLICKED on " << this << ": position" << pos() << "scenePos:" << scenePos() << "event pos" << event->pos();

	//resetTransform();

	//translate();
	//translate(pos().x(), pos().y());
	scale(1.1, 1.1);
	//setScale(1.1);
	//translate(-pos().x(), -pos().y());
	//setPen(QPen(Qt::blue));

	QGraphicsEllipseItem::mousePressEvent(event);
}
