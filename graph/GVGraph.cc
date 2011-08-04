#include "GVGraph.h"

#include <QDebug>

using namespace gv;

/**
 * VERY IMPORTANT:
 *
 * do NOT ever call the *delete() functions without first calling gvFreeLayout(), it will
 * cause horrible problems (memory leaks not the least of them)
 */

/*! Dot uses a 72 DPI value for converting it's position coordinates from points to pixels
 while we display at 96 DPI on most operating systems. */
const double GVGraph::DotDefaultDPI = 72.0;

GVGraph::GVGraph(QString name, QString layout, int type, QFont font, double nodeSize) :
	mContext(gvContext()), mGraph(_agopen(name, type)) {
	//Set graph attributes
	_agset(mGraph, "overlap", "prism"); // the prism algorithm for node/edge overlap reduction is state of the art, recommended.
	//_agset(mGraph, "splines", "true"); // this will only work for very very small graphs, it produces app crashes on bigger ones, disabled
	//_agset(mGraph, "pad", "0.2");
	_agset(mGraph, "dpi", "96.0");
	_agset(mGraph, "nodesep", "0.2");

	//Set default attributes for the future nodes
	_agnodeattr(mGraph, "fixedsize", "true");
	_agnodeattr(mGraph, "label", "");
	//_agnodeattr(mGraph, "regular", "true");

	setGlobalNodeSize(nodeSize);
	setLayoutAlgorithm(layout);
	setFont(font);
}

GVGraph::~GVGraph() {
	gvFreeLayout(mContext, mGraph);
	agclose(mGraph);
	gvFreeContext(mContext);

	qDebug() << "GVGraph::~GVGraph: ran";
}

void GVGraph::addNode(const QString& name) {
	if (!mNodes.contains(name)) {
		// example
		//_agset(node, "width", width);

		mNodes.insert(name, _agnode(mGraph, name));
	}
}

void GVGraph::addNodes(const QStringList& names) {
	foreach (const QString& name, names) {
		addNode(name);
	}
}

void GVGraph::removeNode(const QString& name) {
	if (mNodes.contains(name)) {
		// delete the edges this node is connected to
		QList< QPair<QString, QString> > keys = mEdges.uniqueKeys();

		for (int i = 0; i < keys.size(); ++i) {
			if (keys.at(i).first == name || keys.at(i).second == name) {
				removeEdges(keys.at(i));
			}
		}

		agdelete(mGraph, mNodes[name]);

		mNodes.remove(name);
	}
}

void GVGraph::clearNodes() {
	QList<QString> keys = mNodes.keys();

	gvFreeLayout(mContext, mGraph);

	qDebug() << "GVGraph::clearNodes: freed layout";

	for (int i = 0; i < keys.size(); ++i) {
		removeNode(keys.at(i));
	}

	qDebug() << "GVGraph::clearNodes: removed nodes";

	assert(mNodes.isEmpty());
}

void GVGraph::setRootNode(const QString& name) {
    if (mNodes.contains(name)) {
    	_agset(mGraph, "root", name);
    }
}

void GVGraph::addEdge(const QString &source, const QString &target) {
	if (mNodes.contains(source) && mNodes.contains(target)) {
		QPair<QString, QString> key(source, target);

		/*
		if (!mEdges.contains(key)) {
			mEdges.insert(key, agedge(mGraph, mNodes[source], mNodes[target]));
		}
		*/

		Agedge_t *edge = agedge(mGraph, mNodes[source], mNodes[target]);

		// if this edge pointer already existed then the graph is probably strict
		// in which case agedge returns the same pointer instead of creating a new edge

		if (!mEdges.values(key).contains(edge)) {
			mEdges.insertMulti(key, agedge(mGraph, mNodes[source], mNodes[target]));
		}
	}
}

void GVGraph::removeEdges(const QString &source, const QString &target) {
	removeEdges(QPair<QString, QString> (source, target));
}

void GVGraph::removeEdges(const QPair<QString, QString>& key) {
	qDebug() << "Attempting to delete edge:" << key << "there are" << mEdges.size() << "edges left";

	if (mEdges.contains(key)) {
		qDebug() << key << "exists, there are" << mEdges.values(key).size() << "subedges";

		foreach (Agedge_t *edge, mEdges.values(key)) {
			qDebug() << "Going to delete" << mGraph << edge;
			agdelete(mGraph, edge);
		}

		qDebug() << "Foreach traversed!";

		//agdelete(mGraph, mEdges[key]);

		mEdges.remove(key);
	}

	qDebug() << "succesfully terminated";
}

void GVGraph::setFont(QFont font) {
    mFont = font;

    _agset(mGraph, "fontname", font.family());
    _agset(mGraph, "fontsize", QString("%1").arg(font.pointSizeF()));

    _agnodeattr(mGraph, "fontname", font.family());
    _agnodeattr(mGraph, "fontsize", QString("%1").arg(font.pointSizeF()));

    _agedgeattr(mGraph, "fontname", font.family());
    _agedgeattr(mGraph, "fontsize", QString("%1").arg(font.pointSizeF()));
}

void GVGraph::setLayoutAlgorithm(const QString& algorithm) {
	mLayoutAlgorithm = algorithm;
}

void GVGraph::setGlobalNodeSize(double size) {
	//Divide the wanted width by the DPI to get the value in points
	QString nodePtsWidth = QString("%1").arg(size / _agget(mGraph, "dpi", "96,0").toDouble());

	//GV uses , instead of . for the separator in floats
	//_agnodeattr(mGraph, "width", nodePtsWidth.replace('.', ","));
	Agsym_t *agWidth = _agnodeattr(mGraph, "width", nodePtsWidth);
	Agsym_t *agHeight = _agnodeattr(mGraph, "height", nodePtsWidth);
	//Agsym_t *a_width = _agnodeattr(mGraph, "width", "");
	//_agnodeattr(mGraph, "height", nodePtsWidth);

	//qDebug() << "GVGraph::setGlobalNodeSize: Node size" << size << "(unit unknown) and in points:" << (size / _agget(mGraph, "dpi", "96,0").toDouble()) << "or" << nodePtsWidth;
	//qDebug() << "GVGraph::setGlobalNodeSize:" << agWidth->name << ":" << agWidth->value << "(" << agWidth->index << ")";
}

void GVGraph::applyLayout() {
    gvFreeLayout(mContext, mGraph);
    _gvLayout(mContext, mGraph, mLayoutAlgorithm);
}

QRectF GVGraph::boundingRect() const {
    double dpi = _agget(mGraph, "dpi", "96,0").toDouble();

    return QRectF(
		mGraph->u.bb.LL.x * (dpi/DotDefaultDPI),
		mGraph->u.bb.LL.y * (dpi/DotDefaultDPI),
		mGraph->u.bb.UR.x * (dpi/DotDefaultDPI),
		mGraph->u.bb.UR.y * (dpi/DotDefaultDPI)
	);
}

QList<GVNode> GVGraph::nodes() const {
	QList<GVNode> list;

	const double dpi = _agget(mGraph, "dpi", "96,0").toDouble();
	const double upperRightY = mGraph->u.bb.UR.y;

	for (NodeMap::const_iterator it = mNodes.begin(), end = mNodes.end(); it != end; ++it) {
		const Agnode_t *node = it.value();
		GVNode object;

		//Set the name of the node
		object.name = node->name;

		//Fetch the X coordinate, apply the DPI conversion rate (actual DPI / 72, used by dot)
		double x = node->u.coord.x * (dpi / DotDefaultDPI);

		//qDebug() << "pos:" << (node->u.pos) << "vs. coord" << node->u.coord.x << "|" << node->u.id << "|" << node->u.heapindex << "|" << node->u.hops;

		//Translate the Y coordinate from bottom-left to top-left corner
		double y = (upperRightY - node->u.coord.y) * (dpi / DotDefaultDPI);

		object.centerPos = QPointF(x, y);

		//Transform the width and height from inches to pixels
		object.height = node->u.height * dpi;
		object.width = node->u.width * dpi;

		list << object;
	}

	return list;
}

QList<GVEdge> GVGraph::edges() const {
	QList<GVEdge> list;

	double dpi = _agget(mGraph, "dpi", "96,0").toDouble();

	for (EdgeMap::const_iterator it = mEdges.begin(), end = mEdges.end(); it != end; ++it) {
		const Agedge_t *edge = it.value();
		GVEdge object;

		//Fill the source and target node names
		object.source = edge->tail->name;
		object.target = edge->head->name;

		//Calculate the path from the spline (only one spline, as the graph is strict. If it
		//wasn't, we would have to iterate over the first list too)
		//Calculate the path from the spline (only one as the graph is strict)
		if ((edge->u.spl->list != 0) && (edge->u.spl->list->size % 3 == 1)) {
			//If there is a starting point, draw a line from it to the first curve point
			if (edge->u.spl->list->sflag) {
				object.path.moveTo(
					edge->u.spl->list->sp.x * (dpi / DotDefaultDPI),
					(mGraph->u.bb.UR.y - edge->u.spl->list->sp.y) * (dpi / DotDefaultDPI)
				);
				object.path.lineTo(
					edge->u.spl->list->list[0].x * (dpi / DotDefaultDPI),
					(mGraph->u.bb.UR.y - edge->u.spl->list->list[0].y) * (dpi / DotDefaultDPI)
				);
			}
			else {
				object.path.moveTo(
					edge->u.spl->list->list[0].x * (dpi / DotDefaultDPI),
					(mGraph->u.bb.UR.y - edge->u.spl->list->list[0].y) * (dpi / DotDefaultDPI)
				);
			}

			//Loop over the curve points
			for (int i = 1; i < edge->u.spl->list->size; i += 3) {
				object.path.cubicTo(
					edge->u.spl->list->list[i].x * (dpi / DotDefaultDPI),
					(mGraph->u.bb.UR.y - edge->u.spl->list->list[i].y) * (dpi / DotDefaultDPI),
					edge->u.spl->list->list[i + 1].x * (dpi / DotDefaultDPI),
					(mGraph->u.bb.UR.y - edge->u.spl->list->list[i + 1].y) * (dpi / DotDefaultDPI),
					edge->u.spl->list->list[i + 2].x * (dpi / DotDefaultDPI),
					(mGraph->u.bb.UR.y - edge->u.spl->list->list[i + 2].y) * (dpi / DotDefaultDPI)
				);
			}

			//If there is an ending point, draw a line to it
			if (edge->u.spl->list->eflag) {
				object.path.lineTo(
					edge->u.spl->list->ep.x * (dpi / DotDefaultDPI),
					(mGraph->u.bb.UR.y - edge->u.spl->list->ep.y) * (dpi / DotDefaultDPI)
				);
			}
		}

		list << object;
	}

	return list;
}
