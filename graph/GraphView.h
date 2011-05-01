#ifndef GRAPHVIEW_H_
#define GRAPHVIEW_H_

#include <QGraphicsView>

// we're forward-declaring it to keep information about GVGraph as minimal as possible in the rest of the program
class GVGraph;
class IMatchModel;

class GraphView : public QGraphicsView {
		Q_OBJECT

	public:
		GraphView(QWidget *parent = 0);
		//GraphView(QGraphicsScene *scene, QWidget *parent = 0);
		virtual ~GraphView();

		virtual void setModel(IMatchModel *model);

	public slots:
		void modelChanged();

	protected:
		void wheelEvent(QWheelEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void showEvent(QShowEvent *event);
		//void mousePressEvent(QMouseEvent *event);

	private:
		void scaleView(qreal scaleFactor);

		void generate();
		void draw();

	private:
		GVGraph *mGraph;
		IMatchModel *mModel;

		bool mDirty;
};

#endif /* GRAPHVIEW_H_ */
