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
	clearNodes();
	agclose(mGraph);
	gvFreeContext(mContext);

	qDebug() << "GVGraph::~GVGraph: ran";
}

/*
void GVGraph::addNode(const QString& name, int id) {
	if (!mNodes.contains(name)) {
		// example
		//_agset(node, "width", width);

		//if (id != -1) mStringToIdMap.insert(name, id);

		mNodes.insert(name, _agnode(mGraph, name));
	}
}
*/

void GVGraph::addNode(int id) {
	if (!mNodes.contains(id)) {
		// example
		//_agset(node, "width", width);

		//if (id != -1) mStringToIdMap.insert(name, id);

		mNodes.insert(id, _agnode(mGraph, QString::number(id)));
	}
}

void GVGraph::removeNode(int id) {
	if (mNodes.contains(id)) {
		// delete the edges this node is connected to
		QList<NodeIdPair> keys = mEdges.uniqueKeys();

		for (int i = 0; i < keys.size(); ++i) {
			if (keys.at(i).first == id || keys.at(i).second == id) {
				removeEdges(keys.at(i));
			}
		}

		agdelete(mGraph, mNodes[id]);

		mNodes.remove(id);
	}
}

/*
void GVGraph::addNodes(const QStringList& names) {
	foreach (const QString& name, names) {
		addNode(name);
	}
}
*/

/*
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
*/

void GVGraph::clearNodes() {
	QList<int> keys = mNodes.keys();

	// free all location information added by a layout algorithm first (safe to call multiple times)
	gvFreeLayout(mContext, mGraph);

	qDebug() << "GVGraph::clearNodes: freed layout";

	for (int i = 0; i < keys.size(); ++i) {
		removeNode(keys.at(i));
	}

	qDebug() << "GVGraph::clearNodes: removed nodes";

	assert(mNodes.isEmpty());
}

void GVGraph::setRootNode(int id) {
    if (mNodes.contains(id)) {
    	_agset(mGraph, "root", QString::number(id));
    }
}

void GVGraph::addEdge(int sourceId, int targetId, int edgeId) {
	NodeMap::const_iterator sourceNode = mNodes.constFind(sourceId);
	NodeMap::const_iterator targetNode = mNodes.constFind(targetId);
	NodeMap::const_iterator end = mNodes.constEnd();

	if (sourceNode != end && targetNode != end) {
		NodeIdPair key(sourceId, targetId);

		GVEdgePrivate edge(agedge(mGraph, sourceNode.value(), targetNode.value()), edgeId);
		//Agedge_t *edge = agedge(mGraph, sourceNode.value(), targetNode.value());

		if (!mEdges.values(key).contains(edge)) {
			mEdges.insertMulti(key, edge);
		}
	}
}

/*
void GVGraph::removeEdge(int edgeId) {

}
*/

void GVGraph::removeEdges(int sourceId, int targetId) {
	removeEdges(NodeIdPair(sourceId, targetId));
}

void GVGraph::removeEdges(const QPair<int, int>& idPair) {
	//qDebug() << "Attempting to delete edge:" << key << "there are" << mEdges.size() << "edges left";

	if (mEdges.contains(idPair)) {
		//qDebug() << key << "exists, there are" << mEdges.values(key).size() << "subedges";

		foreach (const GVEdgePrivate& edge, mEdges.values(idPair)) {
			//qDebug() << "Going to delete" << mGraph << edge;
			agdelete(mGraph, edge.edge);
		}

		//qDebug() << "Foreach traversed!";

		//agdelete(mGraph, mEdges[key]);

		mEdges.remove(idPair);
	}

	//qDebug() << "succesfully terminated";
}

/*
void GVGraph::addEdge(const QString &source, const QString &target, int id) {
	if (mNodes.contains(source) && mNodes.contains(target)) {
		QPair<QString, QString> key(source, target);

		//if (!mEdges.contains(key)) mEdges.insert(key, agedge(mGraph, mNodes[source], mNodes[target]));

		Agedge_t *edge = agedge(mGraph, mNodes[source], mNodes[target]);

		// if this edge pointer already existed then the graph is probably strict
		// in which case agedge returns the same pointer instead of creating a new edge

		if (!mEdges.values(key).contains(edge)) {
			mEdges.insertMulti(key, edge);
		}
	}
}

void GVGraph::removeEdges(const QString &source, const QString &target) {
	removeEdges(QPair<QString, QString>(source, target));
}

void GVGraph::removeEdges(const QPair<QString, QString>& key) {
	//qDebug() << "Attempting to delete edge:" << key << "there are" << mEdges.size() << "edges left";

	if (mEdges.contains(key)) {
		//qDebug() << key << "exists, there are" << mEdges.values(key).size() << "subedges";

		foreach (Agedge_t *edge, mEdges.values(key)) {
			//qDebug() << "Going to delete" << mGraph << edge;
			agdelete(mGraph, edge);
		}

		//qDebug() << "Foreach traversed!";

		//agdelete(mGraph, mEdges[key]);

		mEdges.remove(key);
	}

	//qDebug() << "succesfully terminated";
}
*/

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

const QString& GVGraph::layoutAlgorithm() const {
	return mLayoutAlgorithm;
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
    const double dpi = _agget(mGraph, "dpi", "96,0").toDouble();

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

	for (NodeMap::const_iterator it = mNodes.constBegin(), end = mNodes.constEnd(); it != end; ++it) {
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

	const double dpi = _agget(mGraph, "dpi", "96,0").toDouble();
	const double scale = (dpi / DotDefaultDPI);

	for (EdgeMap::const_iterator it = mEdges.constBegin(), end = mEdges.constEnd(); it != end; ++it) {
		GVEdge object;

		const Agedge_t *edge = it.value().edge;

		//Fill the source and target node names
		object.source = edge->tail->name;
		object.target = edge->head->name;
		object.id = it.value().id;

		//Calculate the path from the spline (only one spline, as the graph is strict. If it
		//wasn't, we would have to iterate over the first list too)
		//Calculate the path from the spline (only one as the graph is strict)
		const splines * const spl = ED_spl(edge);
		const bezier * const firstBezier = spl->list;
		const boxf bb = GD_bb(mGraph);

		if ((spl->list != 0) && (spl->list->size % 3 == 1)) {
			if (spl->size > 1) {
				qDebug("GVGraph::edges: %d splines were available but only 1 will be rendered because there is no functionality for multiple splines yet", spl->size);
			}

			//If there is a starting point, draw a line from it to the first curve point
			if (firstBezier->sflag) {
				object.path.moveTo(
					firstBezier->sp.x * scale,
					(bb.UR.y - firstBezier->sp.y) * scale
				);
				object.path.lineTo(
					firstBezier->list[0].x * scale,
					(bb.UR.y - firstBezier->list[0].y) * scale
				);
			}
			else {
				object.path.moveTo(
					firstBezier->list[0].x * scale,
					(bb.UR.y - firstBezier->list[0].y) * scale
				);
			}

			//Loop over the curve points
			for (int i = 1; i < firstBezier->size; i += 3) {
				object.path.cubicTo(
					firstBezier->list[i].x * scale,
					(bb.UR.y - firstBezier->list[i].y) * scale,
					firstBezier->list[i + 1].x * scale,
					(bb.UR.y - firstBezier->list[i + 1].y) * scale,
					firstBezier->list[i + 2].x * scale,
					(bb.UR.y - firstBezier->list[i + 2].y) * scale
				);
			}

			//If there is an ending point, draw a line to it
			if (firstBezier->eflag) {
				object.path.lineTo(
					firstBezier->ep.x * scale,
					(bb.UR.y - firstBezier->ep.y) * scale
				);
			}
		}

		list << object;
	}

	return list;
}
