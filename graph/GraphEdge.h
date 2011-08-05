#ifndef GRAPHEDGE_H_
#define GRAPHEDGE_H_

#include <QGraphicsPathItem>
#include <QPen>
#include <QBrush>

class GraphEdge : public QGraphicsPathItem {
	public:
		GraphEdge(const QPainterPath &path, QGraphicsItem * parent = 0);
		GraphEdge(const QPainterPath &path, const QPen &pen = QPen(), const QBrush &brush = QBrush());
		virtual ~GraphEdge();

		void setInfo(const QString& message);

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void hoverEnterEvent (QGraphicsSceneHoverEvent * event);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent * event);

		void init();

	private:
		QGraphicsEllipseItem *mPopup;
		QGraphicsTextItem *mInfo;
};

#endif /* GRAPHEDGE_H_ */
