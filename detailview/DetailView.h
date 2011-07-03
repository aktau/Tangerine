#ifndef DETAILVIEW_H_
#define DETAILVIEW_H_

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QtOpenGL>

class DetailView: public QGraphicsView {
	public:
		DetailView(QWidget *parent = NULL) : QGraphicsView(parent) {
			setWindowTitle(tr("View details of match"));
			setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
		}

	protected:
		void resizeEvent(QResizeEvent *event) {
			if (scene()) {
				scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
			}

			QGraphicsView::resizeEvent(event);
		}
};

class DetailScene : public QGraphicsScene {
	Q_OBJECT

	public:
		DetailScene(QObject *parent = 0);
		virtual ~DetailScene();

		virtual void drawBackground(QPainter *painter, const QRectF &rect);

	protected:
		void setStates();
		void setLights();
		void defaultStates();

		//virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		//virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
		//virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
		//virtual void wheelEvent(QGraphicsSceneWheelEvent * event);

	private:
	    void initGL();

	private:
	    QGraphicsTextItem *mDescription;
};

#endif /* DETAILVIEW_H_ */
