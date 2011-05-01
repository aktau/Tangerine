#ifndef GVGRAPH_H_
#define GVGRAPH_H_

#include <QMap>
#include <QPair>
#include <QFont>
#include <QString>
#include <QStringList>
#include <QRectF>
#include <QPainterPath>
#include <QList>

// a type in this library conflicts with one we already had and this caused a lot of problems
namespace gv {
	#include "gvc.h"
	#include "graph.h"
};

/// A struct containing the information for a GVGraph's node
struct GVNode {
    /// The unique identifier of the node in the graph
    QString name;

    /// The position of the center point of the node from the top-left corner
    QPointF centerPos;

    /// The size of the node in pixels
    double height, width;

    /// convenience function
    /*
    inline QPoint topLeftPos() const {
    	return QPoint(centerPos.x() - width / 2, centerPos.y() - height / 2);
    }
    */

    inline QRectF rect() const {
		return QRectF(
			centerPos.x() - width / 2.0f,
			centerPos.y() - height / 2.0f,
			width,
			height
		);
	}
};

struct GVEdge {
    /// The source and target nodes of the edge
    QString source;
    QString target;

    /// Path of the edge's line
    QPainterPath path;
};

class GVGraph {
	public:
		/// Default DPI value used by dot (which uses points instead of pixels for coordinates)
		static const double DotDefaultDPI;

		/*!
		 * \brief Construct a Graphviz graph object
		 * \param name The name of the graph, must be unique in the application
		 * \param font The font to use for the graph
		 * \param node_size The size in pixels of each node
		 */
		GVGraph(QString name, QString layout = "dot", int type = AGDIGRAPHSTRICT, QFont font = QFont(), double nodeSize = 50);
		~GVGraph();

		/// Add and remove nodes
		void addNode(const QString& name);
		void addNodes(const QStringList& names);
		void removeNode(const QString& name);
		void clearNodes();

		/// Add and remove edges
		void addEdge(const QString& source, const QString& target);
		void removeEdges(const QString& source, const QString& target);
		void removeEdges(const QPair<QString, QString>& key);

		/// Set the font to use in all the labels
		void setFont(QFont font);
		void setLayoutAlgorithm(const QString& algorithm);
		void setGlobalNodeSize(double size);

		void setRootNode(const QString& name);

		// layouts
		void applyLayout();
		QRectF boundingRect() const;

		// get nodes and edges!
		QList<GVNode> nodes() const;
		QList<GVEdge> edges() const;

	private:
		typedef QMap<QPair<QString, QString> , gv::Agedge_t*> EdgeMap;
		typedef QMap<QString, gv::Agnode_t*> NodeMap;

		gv::GVC_t *mContext;
		gv::Agraph_t *mGraph;
		QFont mFont;
		NodeMap mNodes;
		EdgeMap mEdges;

		QString mLayoutAlgorithm;
};

/// The agopen method for opening a graph
static inline gv::Agraph_t* _agopen(QString name, int kind) {
	return gv::agopen(const_cast<char *> (qPrintable(name)), kind);
}

/// Add an alternative value parameter to the method for getting an object's attribute
static inline QString _agget(void *object, QString attr, QString alt = QString()) {
	QString str = gv::agget(object, const_cast<char *> (qPrintable(attr)));

	if (str.isEmpty()) {
		return alt;
	}
	else {
		return str;
	}
}

/// Directly use agsafeset which always works, contrarily to agset
static inline int _agset(void *object, QString attr, QString value) {
	return gv::agsafeset(
		object,
		const_cast<char *> (qPrintable(attr)),
		const_cast<char *> (qPrintable(value)),
		const_cast<char *> (qPrintable(value))
	);
}

static inline gv::Agsym_t * _agnodeattr(gv::Agraph_t *graph, QString name, QString value) {
	return gv::agnodeattr(
		graph,
		const_cast<char *> (qPrintable(name)),
		const_cast<char *> (qPrintable(value))
	);
}

static inline gv::Agsym_t * _agedgeattr(gv::Agraph_t *graph, QString name, QString value) {
	return gv::agedgeattr(
		graph,
		const_cast<char *> (qPrintable(name)),
		const_cast<char *> (qPrintable(value))
	);
}

static inline int _gvLayout(gv::GVC_t *gvc, gv::Agraph_t *graph, QString engine) {
	return gv::gvLayout(
		gvc,
		graph,
		const_cast<char *> (qPrintable(engine))
	);
}

static inline gv::Agnode_t * _agnode(gv::Agraph_t *graph, QString name) {
	return gv::agnode(
		graph,
		const_cast<char *> (qPrintable(name))
	);
}

#endif /* GVGRAPH_H_ */
