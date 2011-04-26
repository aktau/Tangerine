#include "GraphView.h"

#include <QGraphicsItem>
#include <QMouseEvent>
#include <QDebug>
#include <QStringList>

#include "math.h"

#include "GVGraph.h"
#include "GraphNode.h"

#include "SQLFragmentConf.h"
#include "MatchModel.h"

#define MAXNODES 4000

using namespace thera;

GraphView::GraphView(QWidget *parent) : QGraphicsView(parent), mGraph(NULL), mModel(NULL) {
	/* Create and set scene + attributes */
	QGraphicsScene *scene = new QGraphicsScene(this);
	setScene(scene);

	/* view attributes */
	setDragMode(QGraphicsView::ScrollHandDrag);
	setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	//setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));

	//scale(2,2);

    /* Create graph */
	mGraph = new GVGraph("Tangerine", "sfdp", AGRAPH);

	/*
    QStringList names = QStringList() << "A" << "B" << "C" << "D" << "E" << "F" << "G";

    mGraph->addNodes(names);
    mGraph->addEdge(names[0], names[1]);
    mGraph->addEdge(names[1], names[2]);

    mGraph->addEdge(names[0], names[3]);
    mGraph->addEdge(names[0], names[4]);
    mGraph->addEdge(names[4], names[3]);
    mGraph->addEdge(names[4], names[5]);
    mGraph->addEdge(names[0], names[5]);
    mGraph->addEdge(names[0], names[5]);
    mGraph->addEdge(names[0], names[5]);
    mGraph->addEdge(names[0], names[6]);
    //mGraph->addEdge(names[6], names[0]);

    mGraph->applyLayout();
    */

    /* Create graphicsview */

    drawGraph();

	setModel(&MatchModel::EMPTY);
}

/*
GraphView::GraphView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent) {
	setDragMode(QGraphicsView::ScrollHandDrag);
}
*/

GraphView::~GraphView() {
	delete mGraph;
}

void GraphView::setModel(MatchModel *model) {
	if (model != NULL) {
		mModel = model;

		connect(mModel, SIGNAL(modelChanged()), this, SLOT(modelChanged()));

		modelChanged();
	}
	else {
		qDebug() << "GraphView::setModel: Invalid model";
	}
}

void GraphView::modelChanged() {
	qDebug() << "GraphView::setModel: called";

	generate();
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
	mGraph->clearNodes();

	for (int i = 0, ii = qMin(mModel->size(), MAXNODES); i < ii; ++i) {
		const SQLFragmentConf& conf = mModel->get(i);

		mGraph->addNode(conf.getSourceId());
		mGraph->addNode(conf.getTargetId());

		mGraph->addEdge(conf.getSourceId(), conf.getTargetId());
	}

	mGraph->applyLayout();

	drawGraph();
}

void GraphView::drawGraph() {
	scene()->clear();

    QColor nodeColor = Qt::black;
    QPen nodePen(nodeColor, 0, Qt::SolidLine);
    QPen edgePen(QColor(0, 0, 0, 127), 2, Qt::SolidLine);

	//scene->addText("Hello, world!");

	//scene()->setSceneRect(mGraph->boundingRect());

	foreach (const GVEdge& edge, mGraph->edges()) {
		scene()->addPath(edge.path, edgePen);
	}

	foreach (const GVNode& node, mGraph->nodes()) {
		QAbstractGraphicsShapeItem *item = new GraphNode(node.rect());

		item->setPen(nodePen);

		///QRadialGradient gradient(node.centerPos, node.width / 2, node.centerPos + QPointF(node.width / 4, - node.height / 4));
		//gradient.setColorAt(0, Qt::white);
		//gradient.setColorAt(1, Qt::black);

		QRadialGradient gradient(QPointF(), node.width / 2, QPointF() + QPointF(node.width / 4, - node.height / 4));
		gradient.setColorAt(0, Qt::white);
		gradient.setColorAt(1, Qt::black);

		item->setBrush(QBrush(gradient));

		//item->setPos(node.topLeftPos());

		scene()->addItem(item);

		//qDebug() << "centerPos:" << node.centerPos << "| rect:" << node.rect();

		//QString msg = QString("[%1,%2] -> [width: %3, height: %4]").arg(node.topLeftPos().x()).arg(node.topLeftPos().y()).arg(node.width).arg(node.height);
		//QMessageBox::about(NULL, QString("Wut?"), msg);
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
