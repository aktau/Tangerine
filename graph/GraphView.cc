#include "GraphView.h"

#include <QGraphicsItem>
#include <QMouseEvent>
#include <QDebug>
#include <QStringList>
#include <QElapsedTimer>
#include <QGLWidget>

#include "math.h"

#include "GVGraph.h"
#include "GraphNode.h"
#include "GraphEdge.h"

#include "IFragmentConf.h"
#include "IMatchModel.h"
#include "EmptyMatchModel.h"

#include <limits>

#define MAXNODES 1000

using namespace thera;

GraphView::GraphView(QWidget *parent) : QGraphicsView(parent), mGraph(NULL), mModel(NULL), mDirty(false) {
	/* Create and set scene + attributes */
	QGraphicsScene *scene = new QGraphicsScene(this);
	scene->setBackgroundBrush(QBrush(QColor("#4f4f4f"), Qt::SolidPattern));
	setScene(scene);

	/* view attributes */
	setDragMode(QGraphicsView::ScrollHandDrag);
	setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));

	mGraph = new GVGraph("Tangerine", "sfdp", AGRAPH, QFont(), 200);
	//mGraph = new GVGraph("Tangerine", "neato", AGRAPHSTRICT, QFont(), 200);

	setModel(&EmptyMatchModel::EMPTY);
}

GraphView::~GraphView() {
	delete mGraph;

	qDebug() << "GraphView::~GraphView: ran";
}

void GraphView::setModel(IMatchModel *model) {
	if (model != NULL) {
		if (mModel != NULL) {
			disconnect(mModel, 0, this, 0);
		}

		mModel = model;

		connect(mModel, SIGNAL(modelChanged()), this, SLOT(modelChanged()));

		modelChanged();
	}
	else {
		qDebug() << "GraphView::setModel: NULL model not accepted";
	}
}

QList<QAction *> GraphView::actions() const {
	return mActions;
}

void GraphView::modelChanged() {
	qDebug() << "GraphView::modelChanged: called, active window:" << isActiveWindow() << "| visible:" << isVisible();

	mDirty = true;

	// there's no need to recalculate a lot of stuff if nothing is visible...
	if (isVisible()) {
		generate();
	}
}

void GraphView::wheelEvent(QWheelEvent *event) {
    scaleView(pow((double)2, -event->delta() / (240.0 * 2)));
}

void GraphView::scaleView(qreal scaleFactor) {
	qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();

	if (factor < 0.07 || factor > 100)
		return;

	scale(scaleFactor, scaleFactor);
}

void GraphView::generate() {
	qDebug() << "GraphView::generate: clearing nodes";

	mGraph->clearNodes();

	qDebug() << "GraphView::generate: adding nodes";

	mModel->prefetchHint(0, qMin(mModel->size(), MAXNODES));

	mThicknessModifierAttribute = "error";
	mMinThicknessModifier = std::numeric_limits<double>::max();
	mMaxThicknessModifier = std::numeric_limits<double>::min();;

	for (int i = 0, ii = qMin(mModel->size(), MAXNODES); i < ii; ++i) {
		const IFragmentConf& conf = mModel->get(i);

		//IMatchModel status = getInt("status", IMatchModel::UNKNOWN);

		double thicknessModifier = conf.getDouble(mThicknessModifierAttribute, 0.0);
		mMinThicknessModifier = qMin(mMinThicknessModifier, thicknessModifier);
		mMaxThicknessModifier = qMax(mMaxThicknessModifier, thicknessModifier);

		mGraph->addNode(conf.mFragments[IFragmentConf::SOURCE]);
		mGraph->addNode(conf.mFragments[IFragmentConf::TARGET]);

		mGraph->addEdge(conf.mFragments[IFragmentConf::SOURCE], conf.mFragments[IFragmentConf::TARGET], conf.index());
	}

	qDebug() << "GraphView::generate: applying layout" << mGraph->layoutAlgorithm();

	QElapsedTimer timer;
	timer.start();

	mGraph->applyLayout();

	qDebug() << "GraphView::generate: applied layout in" << timer.restart() << "msec, drawing graph...";

	draw();

	qDebug() << "GraphView::generate: graph drawn in" << timer.elapsed() << "msec";

	fitInView(scene()->sceneRect(), Qt::KeepAspectRatioByExpanding);
}

void GraphView::draw() {
	scene()->clear();

	const QColor nodeStrokeColor = QColor("#8eb650");
    const QColor nodeColor = QColor("#cfe8a7");

    QColor edgeColor = QColor(nodeColor);
    edgeColor.setAlpha(127);

    const QBrush nodeBrush = QColor(nodeColor);
    const QPen nodePen(nodeStrokeColor, 5, Qt::SolidLine);
    const QPen edgePen(edgeColor, 2, Qt::SolidLine);
    //QPen edgePen(QColor(0, 0, 0, 127), 2, Qt::SolidLine);

	//scene->addText("Hello, world!");

	//scene()->setSceneRect(mGraph->boundingRect());

	foreach (const GVEdge& edge, mGraph->edges()) {
		const IFragmentConf *conf = findCorresponding(edge);

		if (conf) {
			QColor c;
			int thickness = 2;

			// TODO: set thickness based on probability/error/...
			double thicknessModifier = conf->getDouble(mThicknessModifierAttribute, 0.0);
			double percentage = (thicknessModifier - mMinThicknessModifier) / (mMaxThicknessModifier - mMinThicknessModifier);

			switch (conf->getInt("status", IMatchModel::UNKNOWN)) {
				case IMatchModel::UNKNOWN: c = QColor(100, 100, 100, 100); break; // unknown
				case IMatchModel::YES: { c = Qt::green; thickness = 15; } break; // correct
				case IMatchModel::MAYBE: { c = QColor(255, 128, 0); thickness = 10; } break; // maybe
				case IMatchModel::NO: { c = Qt::red; thickness = 1; } break; // no
				case IMatchModel::CONFLICT: c = /* Qt::magenta */ QColor(128, 128, 128); break; // no by conflict

				default: c = Qt::white;
			}

			if (!mState.scaleThicknessByStatus) thickness = 2;

			if (mState.drawProbabilities) {
				QColor pc = c;
				pc.setAlpha(100);

				GraphEdge *edgeItem = new GraphEdge(edge.path, QPen(pc, thickness + percentage * 3 * thickness, Qt::SolidLine));
				//edgeItem->setZValue(100);
				scene()->addItem(edgeItem);
				//scene()->addPath(edge.path, QPen(pc, thickness + percentage * 3 * thickness, Qt::SolidLine));
			}

			GraphEdge *edgeItem = new GraphEdge(edge.path, QPen(c, thickness, Qt::SolidLine));
			edgeItem->setInfo("<h1>" + conf->getSourceId() + " / " + conf->getTargetId() + "</h1>");
			//edgeItem->setZValue(100);
			scene()->addItem(edgeItem);
			//scene()->addPath(edge.path, QPen(c, thickness, Qt::SolidLine));
		}
		else {
			scene()->addItem(new GraphEdge(edge.path, edgePen));
			//scene()->addPath(edge.path, edgePen);
		}
	}

	foreach (const GVNode& node, mGraph->nodes()) {
		QAbstractGraphicsShapeItem *item = new GraphNode(node.rect());

		item->setPen(nodePen);
		item->setBrush(nodeBrush);

		//QRadialGradient gradient(QPointF(), node.width / 2, QPointF() + QPointF(node.width / 4, - node.height / 4));
		//gradient.setColorAt(0, Qt::white);
		//gradient.setColorAt(1, Qt::black);

		//item->setBrush(QBrush(gradient));

		//item->setPos(node.topLeftPos());

		scene()->addItem(item);

		float width = node.width / 1.3;
		float height = node.height / 1.3;
		item = new GraphNode(QRectF(
			node.centerPos.x() - width / 2.0f,
			node.centerPos.y() - height / 2.0f,
			width,
			height
		));

		item->setPen(QPen(QColor("#96A879"), 5, Qt::SolidLine));
		item->setBrush(QColor("#FFF0BA"));
		scene()->addItem(item);

		//qDebug() << "centerPos:" << node.centerPos << "| rect:" << node.rect();

		//QString msg = QString("[%1,%2] -> [width: %3, height: %4]").arg(node.topLeftPos().x()).arg(node.topLeftPos().y()).arg(node.width).arg(node.height);
		//QMessageBox::about(NULL, QString("Wut?"), msg);
	}

	mDirty = false;
}

void GraphView::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
		case Qt::Key_Minus:
		{
			static double nodeSize = 400;

			nodeSize /= 2.0;
			mGraph->setGlobalNodeSize(nodeSize);

			generate();
		}
		break;

		case Qt::Key_P:
		{
			mState.drawProbabilities = !mState.drawProbabilities;

			draw();
		}
		break;

		case Qt::Key_S:
		{
			mState.scaleThicknessByStatus = !mState.scaleThicknessByStatus;

			draw();
		}
		break;

		case Qt::Key_L:
		{
			// "twopi" crashes a lot
			static QStringList layouts = QStringList() << "sfdp" << "fdp" << "neato"  << "circo";
			static int index = 0;

			index = (index + 1) % layouts.size();

			//delete mGraph;
			//mGraph = new GVGraph("Tangerine", "neato", AGRAPH, QFont(), 200);

			mGraph->setLayoutAlgorithm(layouts[index]);

			qDebug() << "Switching layout algorithm to:" << layouts[index];

			mGraph->applyLayout();
			draw();
			//generate();
		}
		break;

		default:
			QGraphicsView::keyPressEvent(event);
	}
}

void GraphView::showEvent(QShowEvent *event) {
	if (mDirty) {
		qDebug() << "GraphView::showEvent: shown and dirty, generating...";

		generate();
	}
}

/*
void GraphView::mousePressEvent(QMouseEvent *event) {
	if (QGraphicsItem *item = itemAt(event->pos())) {
		qDebug() << "You clicked on item" << item;
	}
	else {
		qDebug() << "You didn't click on an item.";
	}

	QGraphicsView::mousePressEvent(event);
}
*/

inline const IFragmentConf *GraphView::findCorresponding(const GVEdge& edge) const {
	for (int i = 0, ii = qMin(mModel->size(), MAXNODES); i < ii; ++i) {
		const IFragmentConf& conf = mModel->get(i);

		if (conf.index() == edge.id) {
			return &conf;
		}
	}

	return NULL;
}
