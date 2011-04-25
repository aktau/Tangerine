#ifndef GRAPHVIEW_H_
#define GRAPHVIEW_H_

#include <QGraphicsView>

// we're forward-declaring it to keep information about GVGraph as minimal as possible in the rest of the program
class GVGraph;

class GraphView : public QGraphicsView {
		Q_OBJECT

	public:
		GraphView(QWidget *parent = 0);
		//GraphView(QGraphicsScene *scene, QWidget *parent = 0);
		virtual ~GraphView();

	protected:
		void wheelEvent(QWheelEvent *event);
		//void mousePressEvent(QMouseEvent *event);

	private:
		void scaleView(qreal scaleFactor);

	private:
		GVGraph *mGraph;
};

#endif /* GRAPHVIEW_H_ */
