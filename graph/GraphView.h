#ifndef GRAPHVIEW_H_
#define GRAPHVIEW_H_

#include <QGraphicsView>

// we're forward-declaring it to keep information about GVGraph as minimal as possible in the rest of the program
class GVGraph;
class MatchModel;

class GraphView : public QGraphicsView {
		Q_OBJECT

	public:
		GraphView(QWidget *parent = 0);
		//GraphView(QGraphicsScene *scene, QWidget *parent = 0);
		virtual ~GraphView();

		virtual void setModel(MatchModel *model);

	public slots:
		void modelChanged();

	protected:
		void wheelEvent(QWheelEvent *event);
		//void mousePressEvent(QMouseEvent *event);

	private:
		void scaleView(qreal scaleFactor);
		void generate();
		void drawGraph();

	private:
		GVGraph *mGraph;
		MatchModel *mModel;
};

#endif /* GRAPHVIEW_H_ */
