#include "GraphView.h"

#include <QGraphicsItem>
#include <QMouseEvent>
#include <QDebug>
#include <QStringList>

#include "math.h"

#include "GVGraph.h"
#include "GraphNode.h"

#include "IFragmentConf.h"
#include "IMatchModel.h"
#include "EmptyMatchModel.h"

#define MAXNODES 5000

using namespace thera;

GraphView::GraphView(QWidget *parent) : QGraphicsView(parent), mGraph(NULL), mModel(NULL), mDirty(false) {
	/* Create and set scene + attributes */
	QGraphicsScene *scene = new QGraphicsScene(this);
	scene->setBackgroundBrush(QBrush(QColor("#4f4f4f"), Qt::SolidPattern));
	setScene(scene);

	/* view attributes */
	setDragMode(QGraphicsView::ScrollHandDrag);
	setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	//setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));

	//mGraph = new GVGraph("Tangerine", "neato", AGRAPH, QFont(), 200);
	mGraph = new GVGraph("Tangerine", "neato", AGRAPHSTRICT, QFont(), 200);

	setModel(&EmptyMatchModel::EMPTY);
}

/*
GraphView::GraphView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent) {
	setDragMode(QGraphicsView::ScrollHandDrag);
}
*/

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
		qDebug() << "GraphView::setModel: Invalid model";
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

	for (int i = 0, ii = qMin(mModel->size(), MAXNODES); i < ii; ++i) {
		const IFragmentConf& conf = mModel->get(i);

		//qDebug() << "GraphView::generate: adding new nodes";

		mGraph->addNode(conf.getSourceId());
		mGraph->addNode(conf.getTargetId());

		mGraph->addEdge(conf.getSourceId(), conf.getTargetId());
	}

	qDebug() << "GraphView::generate: applying layout";

	mGraph->applyLayout();

	qDebug() << "GraphView::generate: drawing graph";

	draw();
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
		scene()->addPath(edge.path, edgePen);
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

		case Qt::Key_L:
		{
			static QStringList layouts = QStringList() << "neato" << "fdp" << "sfdp" << "circo" << "twopi";
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
