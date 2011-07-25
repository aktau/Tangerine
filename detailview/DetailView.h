#ifndef DETAILVIEW_H_
#define DETAILVIEW_H_

#include <QObject>
#include <QVector>
#include <QSet>
#include <QMap>
#include <QHash>
#include <QString>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QtOpenGL>
#include <QFuture>
#include <QFutureWatcher>

//#include <vector>

#include "CMesh.h"
#include "XF.h"
#include "GLCamera.h"
#include "TabletopModel.h"

#include "FragmentResources.h"

class DetailScene : public QGraphicsScene {
	Q_OBJECT

	public:
		DetailScene(QObject *parent = 0);
		virtual ~DetailScene();

		virtual void drawBackground(QPainter *painter, const QRectF &rect);

		void init(thera::TabletopModel *tabletopModel);

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
		// WARNING: do not EVER make drawMesh const even though it seems as if
		// one could. The fact of the matter is that when you do this, mMeshColors
		// will also be const and then it's .value()/[] methods start returning
		// copies instead of references, which is a really nice way to waste time
		// bughunting
	    //void drawMesh(const QString& id, thera::Fragment::meshEnum meshType);
		void drawMesh(const FragmentResources& resources);
	    void drawTstrips(const thera::Mesh *themesh) const;

	    void resetView();
	    void updateBoundingSphere();
	    void updateDisplayInformation();

	    void calcMeshData(const QStringList& fragmentList, bool update = true);

	    // removes all meshes that are no longer on the tabletop (or remove all meshes if the tabletop no longer exists)
	    void unloadMeshes();

	    thera::Mesh *getMesh(const thera::PlacedFragment *pf, thera::Fragment::meshEnum meshType) const;
	    thera::Mesh *getMesh(const QString& id, thera::Fragment::meshEnum meshType) const;
	    thera::XF getXF(const thera::PlacedFragment *pf) const;
	    thera::XF getXF(const QString& id) const;

	private slots:
		void calcDone();

	private:
	    thera::XF mGlobalXF;
	    thera::Mesh::BSphere mGlobalBoundingSphere;

	    typedef QWeakPointer<thera::TabletopModel> TabletopPointer;
	    TabletopPointer mTabletopModel;

	    typedef QMap<QString, FragmentResources *> FragmentMap;
		FragmentMap mLoadedFragments;

	    QGraphicsTextItem *mDescription;

	    int mDistanceExponential;

	    float mTranslateX;

	    bool mLoaded;
	    bool mThreaded;
	    QFutureWatcher<void> mWatcher;

	private:
	    struct State {
			int current_mesh;
			int draw_alternate;

			bool draw_ribbon;
			bool draw_edges;
			bool draw_shiny;
			bool draw_lit;
			bool draw_falsecolor;
			bool draw_index;
			bool draw_points;
			bool white_bg;

			bool highQuality;
			bool transparancyEnabled;
			bool drawBothSides;

			float transparancy;

			State(void) :
				current_mesh(-1), draw_alternate(0), draw_ribbon(false),
				draw_edges(false), draw_shiny(false),
				draw_lit(false), draw_falsecolor(false), draw_index(false),
				draw_points(false), white_bg(true), highQuality(false), transparancyEnabled(false), drawBothSides(false),
				transparancy(0.2) {
				// nothing
			}
		};

	    State mState;
};

class DetailView: public QGraphicsView {
	Q_OBJECT

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

		void keyPressEvent(QKeyEvent *event) {
			int key = event->key();
			Qt::KeyboardModifiers keystate = event->modifiers();

			if (keystate == Qt::NoModifier) {
				switch (key) {
					case Qt::Key_Escape:
						close();
					break;
				}
			}

			QGraphicsView::keyPressEvent(event);
		}

		void closeEvent(QCloseEvent *event) {
			if (DetailScene *detailScene = qobject_cast<DetailScene *>(scene())) {
				qDebug() << "DetailView::closeEvent: window closed, unpinning resources";

				// unpin resources
				detailScene->init(NULL);

				qDebug() << "DetailView::closeEvent: window closed, unpinned resources";
			}

			QGraphicsView::closeEvent(event);
		}
};

#endif /* DETAILVIEW_H_ */
