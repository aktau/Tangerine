#ifndef GRAPHVIEW_H_
#define GRAPHVIEW_H_

#include <QGraphicsView>
#include <QList>
#include <QAction>

// we're forward-declaring it to keep information about GVGraph as minimal as possible in the rest of the program
class GVGraph;
class GVEdge;
class GVNode;
class IMatchModel;

namespace thera {
	class IFragmentConf;
}

class GraphView : public QGraphicsView {
		Q_OBJECT

	public:
		GraphView(QWidget *parent = 0);
		//GraphView(QGraphicsScene *scene, QWidget *parent = 0);
		virtual ~GraphView();

		virtual void setModel(IMatchModel *model);

		QList<QAction *> actions() const;

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

		const thera::IFragmentConf *findCorresponding(const GVEdge& edge) const;
		//const thera::IFragmentConf& findCorresponding(const GVNode& node) const;

	private:
		QList<QAction *> mActions;

		GVGraph *mGraph;
		IMatchModel *mModel;

		bool mDirty;

		QString mThicknessModifierAttribute;
		double mMinThicknessModifier, mMaxThicknessModifier;

	private:
		struct State {
			bool drawProbabilities;
			bool scaleThicknessByStatus;

			State() : drawProbabilities(true), scaleThicknessByStatus(true) { }
		};

		State mState;
};

#endif /* GRAPHVIEW_H_ */
