#include "DetailView.h"

using namespace thera;

DetailScene::DetailScene(QObject *parent) : QGraphicsScene(parent), mTabletopModel(NULL), mDistanceExponential(600), mTranslateX(0.0) {
	setSceneRect(0, 0, 800, 600);

	mDescription = new QGraphicsTextItem;
	mDescription->setParent(this);
	mDescription->setPos(QPointF(10.0f, 10.0f));
	mDescription->setDefaultTextColor(Qt::white);
	addItem(mDescription);

	initGL();
}

DetailScene::~DetailScene() {
}

void DetailScene::init(const TabletopModel *tabletopModel) {
	mGlobalXF = XF();

	mTabletopModel = tabletopModel;

	tabletopChanged();
}

void DetailScene::tabletopChanged() {
	//if (!isVisible()) return;
	if (!mTabletopModel) return;

	bool need_resetview = mPinnedFragments.isEmpty();

	foreach (const QString& id, mPinnedFragments) {
		if (!mTabletopModel->contains(id)) {
			Database::fragment(id)->mesh(Fragment::HIRES_MESH).unpin();
			Database::fragment(id)->mesh(Fragment::RIBBON_MESH_FORMAT_STRING).unpin();
			mPinnedFragments.remove(id);
			//mesh_colors.remove(id);
		}
	}

	for (TabletopModel::const_iterator it = mTabletopModel->begin(); it != mTabletopModel->end(); ++it) {
		const PlacedFragment *pf = *it;
		const Fragment *f = pf->fragment();

		if (mPinnedFragments.contains(f->id())) continue;

		f->mesh(Fragment::HIRES_MESH).pin();
		f->mesh(Fragment::RIBBON_MESH_FORMAT_STRING).pin();
		mPinnedFragments.insert(f->id());

		qDebug() << "Calculating for RIBBON_MESH_FORMAT_STRING";
		Mesh *m = &*f->mesh(Fragment::RIBBON_MESH_FORMAT_STRING);
		m->need_normals();
		m->need_tstrips();
		m->need_bsphere();

		qDebug() << "Calculating for HIRES_MESH";
		m = &*f->mesh(Fragment::HIRES_MESH);
		m->need_normals();
		m->need_tstrips();
		m->need_bsphere();

		// now set up the mesh colors properly
		/*
		const MMImage *mmimg = f->color(Fragment::FRONT);
		if (mmimg && m->colors.size()) {
			const CImage &cimg = mmimg->fetchImageFromLevel(0);
			const CImage &cmask = f->masks(Fragment::FRONT);
			AutoPin p1(cimg), p2(cmask);
			Image *img = &(*cimg), *mask = &(*cmask);
			mesh_colors[f->id()] = m->colors;
			vector < Color > &c = mesh_colors[f->id()];
			for (size_t i = 0; i < m->vertices.size(); i++) {
				vec &v = m->vertices[i];
				//vec &n = m->normals[i];  // not used

				// if (n[2] < 0) continue; // only do up-pointing vertices
				if (v[2] < -2)
					continue;
				vec4 mc = mask->bilinMM(v[0], v[1]);
				if (mc[Fragment::FMASK] != 1)
					continue;
				vec4 nc = img->bilinMM(v[0], v[1]);
				// if (nc[3] == 0 || (nc[0] == 0 && nc[1] == 0 & nc[2] == 0)) continue;
				// c[i] = Color(1.0f, 0.0f, 0.0f);
				c[i] = Color(nc[0], nc[1], nc[2]);
			}
		}
		*/
	}

	need_resetview &= !mPinnedFragments.isEmpty();

	if (need_resetview) {
		resetView();
	}

	updateDisplayInformation();
	//need_redraw();
}

void DetailScene::drawBackground(QPainter *painter, const QRectF &) {
	float width = float(painter->device()->width());
	float height = float(painter->device()->height());

	painter->beginNativePainting();
	setStates();

	// GL code here
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	//gluPerspective(60.0, width / height, 0.01, 15.0);
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

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLES);
		glColor3f(1.0f, 0.0f, 0.0f); glVertex3f( 0.0f, 1.0f, 0.0f);
		glColor3f(1.0f, 1.0f, 0.0f); glVertex3f(-1.0f,-1.0f, 0.0f);
		glColor3f(0.0f, 1.0f, 1.0f); glVertex3f( 1.0f,-1.0f, 0.0f);
	glEnd();

	glTranslatef(3.0f,0.0f,0.0f);

	float off = 1.0f;

	glBegin(GL_TRIANGLE_STRIP);
		glColor3f(1.0f, 0.0f, 0.0f); glVertex3f( 0.0f, 1.0f, 0.0f);
		glColor3f(1.0f, 1.0f, 0.0f); glVertex3f(-1.0f,-1.0f, 0.0f);
		glColor3f(0.0f, 1.0f, 1.0f); glVertex3f( 1.0f,-1.0f, 0.0f);
		glColor3f(0.8f, 0.3f, 1.0f);  glVertex3f( 0.0f, -1.0f - off, 0.0f);
	glEnd();

	glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);

	if (mTabletopModel) {
		int i = 0;

		for (TabletopModel::const_iterator it = mTabletopModel->begin(), end = mTabletopModel->end(); it != end; ++it, ++i) {
			//setup_lighting((*it)->id());
			//qDebug() << "Drawing mesh" << i;

			if (mState.draw_alternate > 0 && (i & 1)) continue;
			if (mState.draw_alternate < 0 && !(i & 1)) continue;

			//glColor3f(0.7f, 0.3f, 0.1f);
			//glColor3f(0.2f + (float)i / 2, 0.5f, 1.0f - (float)i / 2);
			glColor4f(0.8f, 0.3f, 1.0f - (float)i / 2, 0.1);
			drawMesh(*it);
		}
	}

	defaultStates();
	painter->endNativePainting();
}

void DetailScene::initGL() {
	//glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
}

void DetailScene::drawMesh(const thera::PlacedFragment *pf) {
	const thera::Mesh *themesh = getMesh(pf);

	glPushMatrix();
	glMultMatrixd(getXF(pf));

	//qDebug() << QString("XF = ") + getXF(pf);

	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	if (mState.draw_2side) {
		glDisable(GL_CULL_FACE);
	} else {
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
	}

	// Vertices
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(themesh->vertices[0]), &themesh->vertices[0][0]);

	// Normals
	if (!themesh->normals.empty() && !mState.draw_index) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(themesh->normals[0]),
				&themesh->normals[0][0]);
	} else {
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	// Colors
	if (!themesh->colors.empty() && !mState.draw_falsecolor
			&& !mState.draw_index && !mState.draw_ribbon) {
		/*
		glEnableClientState(GL_COLOR_ARRAY);
		const float *c = &themesh->colors[0][0];
		if (mesh_colors.contains(id)) {
			c = &mesh_colors[id][0][0];
		}
		*/
		//glColorPointer(3, GL_FLOAT, sizeof(themesh->colors[0]), c /* &themesh->colors[0][0] */);
	}
	else {
		glDisableClientState(GL_COLOR_ARRAY);
	}

	// Main drawing pass
	if (themesh->tstrips.empty() || mState.draw_points) {
	//if (true) {
		// No triangles - draw as points
		glPointSize(1);
		glDrawArrays(GL_POINTS, 0, themesh->vertices.size());
		glPopMatrix();
		return;
	}

	if (mState.draw_edges) {
		glPolygonOffset(10.0f, 10.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}

	drawTstrips(themesh);
	glDisable(GL_POLYGON_OFFSET_FILL);

	// Edge drawing pass
	if (mState.draw_edges) {
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
		glColor3f(0, 0, 1); // Used iff unlit
		drawTstrips(themesh);
		glPolygonMode(GL_FRONT, GL_FILL);
	}

	glPopMatrix();
}

void DetailScene::drawTstrips(const thera::Mesh *themesh) {
	//qDebug() << "Tstrips size =" << themesh->tstrips.size() << "and" << &themesh->tstrips[0] << "|" << themesh->tstrips[0] << "and" << &themesh->tstrips[1] << "|" << themesh->tstrips[1];

	const int *t = &themesh->tstrips[0];
	const int *end = t + themesh->tstrips.size();

	while (likely(t < end)) {
		//qDebug() << t << "<" << end;
		//qDebug() << (int)t << "<" << (int)end;

		int striplen = *t++;

		//qDebug() << "DEREF:" << *(t - 1) << " AND: " << *t;

		glDrawElements(GL_TRIANGLE_STRIP, striplen, GL_UNSIGNED_INT, t);
		t += striplen;
	}
}

void DetailScene::setStates() {
    //glClearColor(0.25f, 0.25f, 0.5f, 1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);

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

void DetailScene::defaultStates() {
    //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glDisable(GL_DEPTH_TEST);
    //glDisable(GL_CULL_FACE);
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

void DetailScene::setLights() {
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
	if (keystate == Qt::NoModifier) {
		switch (key) {
		case Qt::Key_Space:
			resetView();
			break;
		case Qt::Key_Down:
			mDistanceExponential -= 500;
			break;
		case Qt::Key_Up:
			mDistanceExponential += 500;
			break;
		case Qt::Key_Left:
			mTranslateX -= 50;
			break;
		case Qt::Key_Right:
			mTranslateX += 50;
			break;
		case Qt::Key_A:
			mState.draw_alternate = !mState.draw_alternate;
			break;
		case Qt::Key_I:
			mState.draw_alternate = !mState.draw_alternate;
			break;
		case Qt::Key_R:
			mState.draw_ribbon = !mState.draw_ribbon;
			break;
		case Qt::Key_E:
			mState.draw_edges = !mState.draw_edges;
			break;
		case Qt::Key_F:
			mState.draw_falsecolor = !mState.draw_falsecolor;
			break;
		case Qt::Key_L:
			mState.draw_lit = !mState.draw_lit;
			break;
		case Qt::Key_Q:
			// case Qt::Key_Escape:
			// die();
			// break;
		case Qt::Key_S:
			mState.draw_shiny = !mState.draw_shiny;
			break;
		case Qt::Key_W:
			mState.white_bg = !mState.white_bg;
			break;
		case Qt::Key_P:
			mState.draw_points = !mState.draw_points;
			break;
		default:
#if 0
			if (key >= Qt::Key_1 && key <= Qt::Key_9) {
				int m = key - Qt::Key_1;
				toggle_vis(m);
			}
#else
			break;
#endif
		}
	}
	else if (keystate == Qt::ShiftModifier) {
		switch (key) {
		case Qt::Key_2:
			mState.draw_2side = !mState.draw_2side;
			break;
		}
	}

	update();
}

void DetailScene::wheelEvent(QGraphicsSceneWheelEvent *event) {
    QGraphicsScene::wheelEvent(event);
    if (!event->isAccepted()) {
    	qDebug() << event->delta();

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
    //camera.stopspin();

    updateBoundingSphere();

    mGlobalXF = xform::trans(0, 0, -5.0f * mGlobalBoundingSphere.r) * xform::rot(M_PI / 4, -1, 0, 0) * xform::trans(-mGlobalBoundingSphere.center);
}

void DetailScene::updateBoundingSphere() {
	point boxmin(1e38f, 1e38f, 1e38f);
	point boxmax(-1e38f, -1e38f, -1e38f);
	bool someVisible = false;

	// adjust boxmin and boxmax
    if (mTabletopModel) {
		for (TabletopModel::const_iterator it = mTabletopModel->begin(), end = mTabletopModel->end(); it != end; ++it) {
			XF xf = getXF(*it);
			Mesh *m = getMesh(*it);

			someVisible = true;

			point c = xf * m->bsphere.center;
			float r = m->bsphere.r;

			for (int j = 0; j < 3; j++) {
				boxmin[j] = qMin(boxmin[j], c[j] - r);
				boxmax[j] = qMax(boxmax[j], c[j] + r);
			}
		}
	}

	if (!someVisible) return;

	point &gc = mGlobalBoundingSphere.center;
	float &gr = mGlobalBoundingSphere.r;
	gc = 0.5f * (boxmin + boxmax);
	gr = 0.0f;

	// adjust bounding sphere center and radius
	if (mTabletopModel) {
		for (TabletopModel::const_iterator it = mTabletopModel->begin(), end = mTabletopModel->end(); it != end; ++it) {
			XF xf = getXF(*it);
			Mesh *m = getMesh(*it);

			point c = xf * m->bsphere.center;
			float r = m->bsphere.r;

			gr = qMax(gr, dist(c, gc) + r);
		}
	}

	qDebug() << "DetailScene::updateBoundingSphere: Global bounding sphere updated";
}

void DetailScene::updateDisplayInformation() {
	QString match, xf;

	for (TabletopModel::const_iterator it = mTabletopModel->begin(), end = mTabletopModel->end(); it != end; ++it) {
		match += (*it)->id();

		XF t = getXF(*it);

		xf = QString() + t;
	}

	mDescription->setHtml(
		QString(
			"<h1>Detailed match information</h1> "
			"<b>Showing match %1</b>"
			"<hr />"
			"<h2>Properties</h2>"
			"<ul><li>Error: %2</li><li>Volume: %3</li></ul> "
			"<p>XF:</p><pre>%4</pre>"
		).arg(match).arg(0.9812).arg(14.5).arg(xf)
	);
}

Mesh *DetailScene::getMesh(const PlacedFragment *pf) {
	return pf ? &*pf->fragment()->mesh(Fragment::HIRES_MESH) : NULL;
}

XF DetailScene::getXF(const PlacedFragment *pf) {
	return pf ? pf->accumXF() : XF();
}
