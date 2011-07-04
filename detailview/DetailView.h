#ifndef DETAILVIEW_H_
#define DETAILVIEW_H_

#include <QObject>
#include <QVector>
#include <QSet>
#include <QString>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QtOpenGL>

#include "CMesh.h"
#include "XF.h"
#include "GLCamera.h"
#include "TabletopModel.h"

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

		void init(const thera::TabletopModel *tabletopModel);

	public slots:
		void tabletopChanged();

	protected:
		void setStates();
		void setLights();
		void defaultStates();

		virtual void keyPressEvent(QKeyEvent *event);

		//virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		//virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
		//virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
		virtual void wheelEvent(QGraphicsSceneWheelEvent * event);

	private:
	    void initGL();

	    void drawMesh(const thera::PlacedFragment *pf);
	    void drawTstrips(const thera::Mesh *themesh);

	    void resetView();
	    void updateBoundingSphere();
	    void updateDisplayInformation();

	    thera::Mesh *getMesh(const thera::PlacedFragment *pf);
	    thera::XF getXF(const thera::PlacedFragment *pf);

	private:
	    thera::XF mGlobalXF;
	    thera::Mesh::BSphere mGlobalBoundingSphere;

	    const thera::TabletopModel *mTabletopModel;

	    QSet<QString> mPinnedFragments;
	    QVector<xform> mXforms;
	    QVector<thera::Mesh *> mMeshes;

	    QGraphicsTextItem *mDescription;

	    int mDistanceExponential;

	private:
	    struct State {
			int current_mesh;
			int draw_alternate;

			bool draw_ribbon;
			bool draw_edges;
			bool draw_2side;
			bool draw_shiny;
			bool draw_lit;
			bool draw_falsecolor;
			bool draw_index;
			bool draw_points;
			bool white_bg;

			State(void) :
				current_mesh(-1), draw_alternate(0), draw_ribbon(false),
				draw_edges(false), draw_2side(true), draw_shiny(false),
				draw_lit(false), draw_falsecolor(false), draw_index(false),
				draw_points(false), white_bg(true) {
				// nothing
			}
		};

	    State mState;
};

#endif /* DETAILVIEW_H_ */
