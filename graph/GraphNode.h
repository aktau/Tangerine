#ifndef GRAPHNODE_H_
#define GRAPHNODE_H_

#include <QGraphicsEllipseItem>

class GraphNode : public QGraphicsEllipseItem {
	public:
		GraphNode(const QRectF& rect, QGraphicsItem *parent = 0);
		GraphNode(qreal x, qreal y, qreal width, qreal height, QGraphicsItem * parent = 0);
		virtual ~GraphNode();

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
};

static inline QRectF centerRect(const QRectF& rect) {
	return QRectF(
		-rect.width() / 2,
		-rect.height() / 2,
		rect.width(),
		rect.height()
	);
}


#endif /* GRAPHNODE_H_ */
