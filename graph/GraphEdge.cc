#include "GraphEdge.h"

#include <QtGui>
#include <QDebug>

GraphEdge::GraphEdge(const QPainterPath &path, QGraphicsItem *parent) : QGraphicsPathItem(path, parent) {
	init();
}

GraphEdge::GraphEdge(const QPainterPath &path, const QPen &pen, const QBrush &brush) : QGraphicsPathItem(path, 0) {
	setPen(pen);
	setBrush(brush);

	init();
}

GraphEdge::~GraphEdge() {
	//qDebug() << "~GraphEdge..." << mPopup;
}

void GraphEdge::init() {
	setAcceptHoverEvents(true);

	mInfo = NULL;

	mPopup = new QGraphicsEllipseItem(this);
	mPopup->hide();
	mPopup->setRect(QRect(0,0,50,50));
}

void GraphEdge::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	qDebug() << "EDGE: CLICKED on: position" << pos() << "scenePos:" << scenePos() << "event pos" << event->pos();

	//resetTransform();

	//translate();
	//translate(pos().x(), pos().y());
	scale(1.1, 1.1);
	//setScale(1.1);
	//translate(-pos().x(), -pos().y());
	//setPen(QPen(Qt::blue));

	QGraphicsPathItem::mousePressEvent(event);
}

void GraphEdge::hoverEnterEvent(QGraphicsSceneHoverEvent * event) {
	qDebug() << "Hovered enter!" << this << event->pos();

	if (mInfo) {
		if (!mInfo->scene()) scene()->addItem(mInfo);
		//mInfo->adjustSize();
		mInfo->setPos(event->pos());
		mInfo->show();
	}

	//mPopup->show();
	//mPopup->setPos(event->pos());

	qDebug() << "Popup:" << mPopup;
}

void GraphEdge::hoverLeaveEvent(QGraphicsSceneHoverEvent * event) {
	qDebug() << "Hovered leave" << this << event->pos();

	if (mInfo) {
		mInfo->hide();
	}

	//mPopup->hide();
}

void GraphEdge::setInfo(const QString& message) {
	delete mInfo;

	mInfo = new QGraphicsTextItem();
	mInfo->setHtml(message);
	mInfo->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	//mInfo->setZValue(10005000);
	mInfo->hide();

	//scene()->addItem(new QGraphicsTextItem(message));
}
