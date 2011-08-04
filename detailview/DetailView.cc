#include "DetailView.h"

#include "Color.h"

using namespace thera;

DetailScene::DetailScene(QObject *parent) : QGraphicsScene(parent), mDistanceExponential(5040), mTranslateX(0.0), mThreaded(false) {
	setSceneRect(0, 0, 800, 600);

	mDescription = new QGraphicsTextItem;
	mDescription->setParent(this);
	mDescription->setPos(QPointF(10.0f, 10.0f));
	mDescription->setDefaultTextColor(Qt::white);
	addItem(mDescription);

	connect(&mWatcher, SIGNAL(finished()), this, SLOT(calcDone()));
}

DetailScene::~DetailScene() {
	// this will effectively unpin() all held meshes
	// all other resources are on the stack so automatically destroyed
	init(NULL);
}

void DetailScene::init(TabletopModel *tabletopModel) {
	if (mTabletopModel.data() != tabletopModel) {
		if (!mTabletopModel.isNull()) disconnect(mTabletopModel.data(), 0, this, 0);

		mGlobalXF = XF();
		mTabletopModel = TabletopPointer(tabletopModel);

		if (!mTabletopModel.isNull()) connect(mTabletopModel.data(), SIGNAL(tabletopChanged()), this, SLOT(tabletopChanged()));

		tabletopChanged();
	}
}

void DetailScene::tabletopChanged() {
	unloadMeshes();

	//if (!isVisible()) return;
	if (!mTabletopModel) return;

	bool needResetView = false;

	QStringList fragmentList;

	TabletopModel *model = mTabletopModel.data();
	for (TabletopModel::const_iterator it = model->begin(), end = model->end(); it != end; ++it) {
		const QString id = (*it)->id();

		if (!mLoadedFragments.contains(id)) {
			needResetView = true;

			fragmentList << id;
		}
	}

	if (mThreaded) {
		// the methods called from calcMeshData use OpenMP, the current combination Qt 4.7 and GCC 4.4 on windows
		// doesn't play nice with that. It's very likely that this bug will be fixed in future versions. There
		// are reports that it's been fixed for GCC 4.5.x and up.
		QFuture<void> future = QtConcurrent::run(this, &DetailScene::calcMeshData, fragmentList, true);
		mWatcher.setFuture(future);
	}
	else {
		updateDisplayInformation();

		QApplication::processEvents();

		calcMeshData(fragmentList);
	}

	if (needResetView) {
		//qDebug() << "DetailScene::tabletopChanged: resetting view";

		resetView();
		update();

		//qDebug() << "DetailScene::tabletopChanged: Updated";
	}
}

void DetailScene::calcMeshData(const QStringList& fragmentList, bool updatePerFragment) {
	foreach (const QString& id, fragmentList) {
		if (!mLoadedFragments.contains(id)) {
			qDebug() << "DetailScene::calcMeshData: inserting" << id;
			mLoadedFragments.insert(id, new FragmentResources(id, Fragment::LORES_MESH));
			//mLoadedFragments.insert(id, new FragmentResources(id, mState.highQuality ? Fragment::HIRES_MESH : Fragment::LORES_MESH));
			//FragmentResources *resources = new FragmentResources(id, Fragment::LORES_MESH);
			//mLoadedFragments.insert(id, resources);
		}
		else {
			qDebug() << "DetailScene::calcMeshData: altering" << id;
			mLoadedFragments[id]->loadOnly(mState.highQuality ? Fragment::HIRES_MESH : Fragment::LORES_MESH);
		}

		if (updatePerFragment) {
			update();
			QApplication::processEvents();
		}
	}

	if (mState.highQuality) {
		foreach (const QString& id, fragmentList) {
			mLoadedFragments[id]->loadOnly(Fragment::HIRES_MESH);
		}

		if (updatePerFragment) {
			update();
			QApplication::processEvents();
		}
	}
}

void DetailScene::unloadMeshes() {
	qDebug() << "DetailScene::unloadMeshes: unloadmeshes begin";

	foreach (const QString& id, mLoadedFragments.keys()) {
		if (!mTabletopModel || !mTabletopModel.data()->contains(id)) {
			FragmentResources *resources = mLoadedFragments.value(id);
			mLoadedFragments.remove(id);
			delete resources;
		}
	}

	qDebug() << "DetailScene::unloadMeshes: unloadmeshes end";
}

void DetailScene::drawBackground(QPainter *painter, const QRectF &) {
	//qDebug() << "DetailScene::drawBackground: begin!";

	if (!mTabletopModel) unloadMeshes();

	float width = float(painter->device()->width());
	float height = float(painter->device()->height());

	painter->beginNativePainting();
	setStates();

	// GL code here
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	gluPerspective(60.0, width / height, 0.01, 1000.0);

	glMatrixMode(GL_MODELVIEW);
	//glTranslatef(-1.5f,0.0f,-6.0f);

	QMatrix4x4 view;
	view.rotate(QQuaternion());
	view.translate(mTranslateX, 0.0);
	view(2, 3) -= 2.0f * exp(mDistanceExponential / 1200.0f);
	//loadMatrix(view);
    // static to prevent glLoadMatrixf to fail on certain drivers
    static GLfloat mat[16];
    const qreal *data = view.constData();
    for (int index = 0; index < 16; ++index) mat[index] = data[index];
    glLoadMatrixf(mat);

	/*
	glBegin(GL_TRIANGLES);
		glColor4f(1.0f, 0.0f, 0.0f, 0.5f); glVertex3f( 0.0f, 1.0f, 0.0f);
		glColor3f(1.0f, 1.0f, 0.0f); glVertex3f(-1.0f,-1.0f, 0.0f);
		glColor4f(0.0f, 1.0f, 1.0f, 1.0f); glVertex3f( 1.0f,-1.0f, 0.0f);
	glEnd();

	glTranslatef(3.0f,0.0f,0.0f);

	float off = 1.0f;

	glBegin(GL_TRIANGLE_STRIP);
		glColor3f(1.0f, 0.0f, 0.0f); glVertex3f( 0.0f, 1.0f, 0.0f);
		glColor3f(1.0f, 1.0f, 0.0f); glVertex3f(-1.0f,-1.0f, 0.0f);
		glColor3f(0.0f, 1.0f, 1.0f); glVertex3f( 1.0f,-1.0f, 0.0f);
		glColor3f(0.8f, 0.3f, 1.0f); glVertex3f( 0.0f, -1.0f - off, 0.0f);
	glEnd();
	*/

	int i = 0;

	qDebug("DetailScene::drawBackground: drawing %d meshes", mLoadedFragments.size());

	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	if (mState.drawBothSides) {
		glDisable(GL_CULL_FACE);
	}
	else {
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
	}

	foreach (const FragmentResources *data, mLoadedFragments) {
		//setup_lighting((*it)->id());
		glColor4f(0.8f, 0.3f, 1.0f - (float)i / 2, 1.0f - mState.transparancy);
		drawMesh(*data);
	}

	defaultStates();
	painter->endNativePainting();

	//qDebug() << "DetailScene::drawBackground: end" << mTabletopModel << "|" << mLoadedFragments.size();
}

//void DetailScene::drawMesh(const QString& id, Fragment::meshEnum meshType) {
void DetailScene::drawMesh(const FragmentResources& resources) {
	//qDebug() << "DetailScene::drawMesh: drawing a mesh!";

	const Mesh *mesh = getMesh(resources.id, resources.activeMesh);

	if (!mesh) {
		qDebug() << "DetailScene::drawMesh: mesh was NULL";

		return;
	}

	glPushMatrix();
	glMultMatrixd(getXF(resources.id));

	// Vertices
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(mesh->vertices[0]), &mesh->vertices[0][0]);

	// Normals
	if (!mesh->normals.empty() && !mState.draw_index) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(mesh->normals[0]), &mesh->normals[0][0]);
	} else {
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	// Colors
	if (!resources.colors.empty() && !mState.draw_falsecolor) {
		const float *c = &resources.colors[0][0];

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_FLOAT, sizeof(mesh->colors[0]), c);
	}
	else {
		glDisableClientState(GL_COLOR_ARRAY);
	}

	// Main drawing pass
	if (mesh->tstrips.empty() || mState.draw_points) {
	//if (true) {
		// No triangles - draw as points
		glPointSize(1);
		glDrawArrays(GL_POINTS, 0, mesh->vertices.size());
		glPopMatrix();
		return;
	}

	if (mState.draw_edges) {
		glPolygonOffset(10.0f, 10.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	drawTstrips(mesh);
	glDisable(GL_POLYGON_OFFSET_FILL);

	// Edge drawing pass
	if (mState.draw_edges) {
		glPolygonMode(GL_FRONT, GL_LINE);
		glDisableClientState(GL_COLOR_ARRAY);

		/*
		glPolygonMode(GL_FRONT, GL_LINE);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisable(GL_COLOR_MATERIAL);
		GLfloat global_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
		GLfloat light0_diffuse[] = { 0.8f, 0.8f, 0.8f, 0.0f };
		GLfloat light1_diffuse[] = { -0.2f, -0.2f, -0.2f, 0.0f };
		GLfloat light0_specular[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
		GLfloat mat_diffuse[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat_diffuse);
		*/

		glColor3f(0, 0, 1); // Used iff unlit
		drawTstrips(mesh);
		glPolygonMode(GL_FRONT, GL_FILL);
	}

	glPopMatrix();
}

inline void DetailScene::drawTstrips(const thera::Mesh *themesh) const {
	//qDebug() << "Tstrips size =" << themesh->tstrips.size() << "and" << &themesh->tstrips[0] << "|" << themesh->tstrips[0] << "and" << &themesh->tstrips[1] << "|" << themesh->tstrips[1];

	const int *t = &themesh->tstrips[0];
	const int *end = t + themesh->tstrips.size();

	while (likely(t < end)) {
		int striplen = *t++;

		glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
		t += striplen;
	}
}

inline void DetailScene::setStates() {
    //glClearColor(0.25f, 0.25f, 0.5f, 1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	//glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);

    if (mState.transparancyEnabled) {
    	//glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA);
    	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
		glEnable(GL_BLEND);
    }
    else {
    	glDisable(GL_BLEND);
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    setLights();

    float materialSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);
}

inline void DetailScene::defaultStates() {
	// Qt manages to error out in some corner cases if we don't disable these explicitly
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHT0);
    glDisable(GL_NORMALIZE);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 0.0f);
    float defaultMaterialSpecular[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defaultMaterialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
}

inline void DetailScene::setLights() {
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    	//float lightColour[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float lightDir[] = {0.0f, 0.0f, 1.0f, 0.0f};
    	//glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColour);
    	//glLightfv(GL_LIGHT0, GL_SPECULAR, lightColour);
    glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
    glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);
    glEnable(GL_LIGHT0);
}

void DetailScene::keyPressEvent(QKeyEvent *event) {
	int key = event->key();
	Qt::KeyboardModifiers keystate = event->modifiers();

	if ((keystate & ~(Qt::KeypadModifier | Qt::NoModifier)) == Qt::NoModifier) {

		switch (key) {
		case Qt::Key_Space: resetView(); break;
		case Qt::Key_Down: mDistanceExponential -= 500; break;
		case Qt::Key_Up: mDistanceExponential += 500; break;
		case Qt::Key_Left: mTranslateX += 50; break;
		case Qt::Key_Right: mTranslateX -= 50; break;
		case Qt::Key_Plus: mState.transparancy = qMin(1.0, qMax(0.0, mState.transparancy + 0.1)); break;
		case Qt::Key_Minus: mState.transparancy = qMin(1.0, qMax(0.0, mState.transparancy - 0.1)); break;
		case Qt::Key_A: mState.draw_alternate = !mState.draw_alternate; break;
		case Qt::Key_D: Cache::instance()->print(); break;
		case Qt::Key_C: Cache::instance()->minimizeSize(); break;
		case Qt::Key_H: {
			mState.highQuality = !mState.highQuality;

			calcMeshData(mLoadedFragments.keys(), true);
		} break;
		case Qt::Key_R: mState.draw_ribbon = !mState.draw_ribbon; break;
		case Qt::Key_E: mState.draw_edges = !mState.draw_edges; break;
		case Qt::Key_T: mState.transparancyEnabled = !mState.transparancyEnabled; break;
		case Qt::Key_F: mState.draw_falsecolor = !mState.draw_falsecolor; break;
		case Qt::Key_L: mState.draw_lit = !mState.draw_lit; break;
		case Qt::Key_S: mState.draw_shiny = !mState.draw_shiny; break;
		case Qt::Key_W: mState.white_bg = !mState.white_bg; break;
		case Qt::Key_P: mState.draw_points = !mState.draw_points; break;
		default:
			//qDebug() << "Unrecognized key:" << key << "vs" << Qt::Key_Plus << "and" << Qt::Key_Minus;
			break;
		}
	}
	else if (keystate == Qt::ShiftModifier) {
		switch (key) {
		case Qt::Key_2:
			mState.drawBothSides = !mState.drawBothSides;
			break;
		}
	}

	updateDisplayInformation();
	update();
}

void DetailScene::wheelEvent(QGraphicsSceneWheelEvent *event) {
    QGraphicsScene::wheelEvent(event);
    if (!event->isAccepted()) {
        mDistanceExponential += event->delta();

        /*
        if (mDistanceExponential < -8 * 120)
            mDistanceExponential = -8 * 120;
        if (mDistanceExponential > 10 * 120)
            mDistanceExponential = 10 * 120;
		*/
        event->accept();

        update();
    }
}

void DetailScene::resetView() {
	//qDebug() << "DetailScene::resetView: begin";
    //camera.stopspin();

    updateBoundingSphere();

    mGlobalXF = xform::trans(0, 0, -5.0f * mGlobalBoundingSphere.r) * xform::rot(M_PI / 4, -1, 0, 0) * xform::trans(-mGlobalBoundingSphere.center);

    qDebug() << "DetailScene::resetView: end";
}

void DetailScene::updateBoundingSphere() {
	qDebug() << "DetailScene::updateBoundingSphere: Going to update bounding sphere info";

	point boxmin(1e38f, 1e38f, 1e38f);
	point boxmax(-1e38f, -1e38f, -1e38f);
	bool someVisible = false;

	// adjust boxmin and boxmax
	for (FragmentMap::const_iterator it = mLoadedFragments.constBegin(), end = mLoadedFragments.constEnd(); it != end; ++it) {
		Mesh *m = getMesh(it.key(), it.value()->activeMesh);
		XF xf = getXF(it.key());

		if (!m) {
			qDebug() << "DetailScene::updateBoundingSphere: encountered NULL mesh for id" << it.key();

			continue;
		}

		someVisible = true;

		point c = xf * m->bsphere.center;
		float r = m->bsphere.r;

		for (int j = 0; j < 3; j++) {
			boxmin[j] = qMin(boxmin[j], c[j] - r);
			boxmax[j] = qMax(boxmax[j], c[j] + r);
		}
	}

	if (!someVisible) return;

	point &gc = mGlobalBoundingSphere.center;
	float &gr = mGlobalBoundingSphere.r;
	gc = 0.5f * (boxmin + boxmax);
	gr = 0.0f;

	// adjust bounding sphere center and radius
	for (FragmentMap::const_iterator it = mLoadedFragments.constBegin(), end = mLoadedFragments.constEnd(); it != end; ++it) {
		Mesh *m = getMesh(it.key(), it.value()->activeMesh);
		XF xf = getXF(it.key());

		if (!m) {
			qDebug() << "DetailScene::updateBoundingSphere: encountered NULL mesh for id" << it.key();

			continue;
		}

		point c = xf * m->bsphere.center;
		float r = m->bsphere.r;

		gr = qMax(gr, dist(c, gc) + r);
	}

	qDebug() << "DetailScene::updateBoundingSphere: Global bounding sphere updated";
}

void DetailScene::updateDisplayInformation() {
	//qDebug() << "DetailScene::updateDisplayInformation: begin";

	QString match, xf;

	if (!mTabletopModel.isNull()) {
		TabletopModel *model = mTabletopModel.data();
		for (TabletopModel::const_iterator it = model->begin(), end = model->end(); it != end; ++it) {
			match += (*it)->id() + (it != end - 1 ? ", " : "");

			XF t = getXF(*it);

			xf = QString() + t;
		}
	}

	QString html = QString(
		"<h1>Detailed match information</h1> "
		"<b>Showing fragments %1</b>"
		"<hr />"
		"<ul>"
		"<li>Quality: <b>%2</b> (press 'h' to change)</li>"
		"<li>Zoom: <b>%3</b> (use the scroll button)</li>"
		"</ul>"
	).arg(match).arg(mState.highQuality ? "high" : "low").arg(mDistanceExponential);

	if (mWatcher.isRunning()) {
		html = QString("<h1>Loading data, please be patient</h1>") + html;
	}

	mDescription->setHtml(html);
}

inline Mesh *DetailScene::getMesh(const PlacedFragment *pf, Fragment::meshEnum meshType) const {
	//qDebug() << "DetailScene::getMesh 2: begin";

	return pf ? &*pf->fragment()->mesh(meshType) : NULL;
}

inline Mesh *DetailScene::getMesh(const QString& id, Fragment::meshEnum meshType) const {
	//qDebug() << "DetailScene::getMesh 1: begin";

	return !mTabletopModel.isNull() ? getMesh(mTabletopModel.data()->placedFragment(id), meshType) : NULL;
}

inline XF DetailScene::getXF(const PlacedFragment *pf) const {
	//qDebug() << "DetailScene::getXF 2: begin";

	return pf ? pf->accumXF() : XF();
}

inline XF DetailScene::getXF(const QString& id) const {
	//qDebug() << "DetailScene::getXF 1: begin";

	return !mTabletopModel.isNull() ? getXF(mTabletopModel.data()->placedFragment(id)) : XF();
}

void DetailScene::calcDone() {
	updateBoundingSphere();
	updateDisplayInformation();
	update();
}
