#include "MatchTileView.h"

#include <QClipboard>
#include <QWidgetAction>
#include <QApplication>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDebug>
#include <QInputDialog>
#include <QPair>
#include <QHBoxLayout>
#include <QToolButton>
#include <QStyle>

#include "Database.h"
#include "Fragment.h"
#include "FragmentRef.h"
//#include "TabletopModel.h"
#include "TabletopIO.h"

#include "EmptyMatchModel.h"
#include "ShowStatusDialog.h"

using namespace thera;

#define THUMB_WIDTH 722
#define THUMB_HEIGHT 466
#define THUMB_GUTTER 10

MatchTileView::MatchTileView(const QDir& thumbDir, QWidget *parent, int rows, int columns, float scale) :
		QScrollArea(parent), mThumbDir(thumbDir), mModel(NULL), mSelectionModel(NULL), mScale(scale), mDetailScene(this) {
	setFrameShape(QFrame::NoFrame);
	setObjectName("MainScrollArea");
	setFocusPolicy(Qt::StrongFocus);
	setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
	setWidgetResizable(true);

	mFrame = new QFrame(NULL);
	mFrame->setFrameShape(QFrame::NoFrame);
	mFrame->setMinimumSize(
		columns * THUMB_WIDTH  * scale + (columns - 1) * THUMB_GUTTER,
		rows * THUMB_HEIGHT * scale + (rows - 1) * THUMB_GUTTER
	);
	mFrame->setObjectName("MainFrame");
	mFrame->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
	mFrame->setStyleSheet("QFrame#MainFrame { background-color: black; }");

	setWidget(mFrame);

	// the order of the following 2 statements is important
	mNumThumbs = rows * columns;
	mThumbs.resize(mNumThumbs);

	mStates.append(State(mNumThumbs));

	for (int row = 0, i = 0; row < rows; row++) {
		for (int col = 0; col < columns; col++, i++) {
			mThumbs[i] = new ThumbLabel(i);
			mThumbs[i]->setFixedSize(THUMB_WIDTH * mScale, THUMB_HEIGHT * mScale);
			mThumbs[i]->setParent(mFrame);
			mThumbs[i]->move(col * (THUMB_WIDTH * mScale + THUMB_GUTTER), row * (THUMB_HEIGHT * mScale + THUMB_GUTTER));

			connect(mThumbs[i], SIGNAL(clicked(int, QMouseEvent *)), this, SLOT(clicked(int, QMouseEvent *)));
			connect(mThumbs[i], SIGNAL(doubleClicked(int, QMouseEvent *)), this, SLOT(doubleClicked(int, QMouseEvent *)));
		}
	}

	createActions();
	createStatusWidgets();

	mStatusMenu = new QMenu(this);

	for (int i = 0; i < IMatchModel::NUM_STATUSES; ++i) {
		mStatusMenuActions << mStatusMenu->addAction(IMatchModel::STATUS_STRINGS[i]);
	}
	connect(mStatusMenu, SIGNAL(triggered(QAction *)), this, SLOT(statusMenuTriggered(QAction *)));

	mStatusMenu->addSeparator();
	mStatusMenu->addAction(mCopyAction);

	mStatusMenu->addSeparator();
	mStatusMenu->addAction(mCommentAction);

	mStatusMenu->addSeparator();
	mStatusMenu->addAction(mFindDuplicatesAction);
	mStatusMenu->addAction(mFindConflictingAction);
	mStatusMenu->addAction(mFindNonconflictingAction);

	setModel(&EmptyMatchModel::EMPTY);
	setSelectionModel(new MatchSelectionModel(model(), this));

#ifdef WITH_DETAILVIEW
	QGLWidget *widget = new QGLWidget(QGLFormat(QGL::SampleBuffers));

	widget->makeCurrent(); // The current context must be set before calling Scene's constructor
	mDetailView.setViewport(widget);
	mDetailView.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	mDetailView.setScene(&mDetailScene);
#endif
}

MatchTileView::~MatchTileView() {

}

void MatchTileView::setModel(IMatchModel *model) {
	if (model != NULL) {
		if (mModel != NULL) {
			disconnect(mModel, 0, this, 0);
		}

		mModel = model;

		connect(mModel, SIGNAL(modelChanged()), this, SLOT(modelChanged()));
		connect(mModel, SIGNAL(orderChanged()), this, SLOT(modelOrderChanged()));

		modelChanged();
	}
	else {
		qDebug() << "MatchTileView::setModel: Invalid model";
	}
}

IMatchModel *MatchTileView::model() const {
	return mModel;
}

void MatchTileView::setSelectionModel(MatchSelectionModel *model) {
	if (model != NULL) {
		if (mSelectionModel != NULL) {
			disconnect(mSelectionModel, 0, this, 0);
		}

		mSelectionModel = model;

		connect(mSelectionModel, SIGNAL(selectionChanged(const QList<int>&, const QList<int>&)), this, SLOT(selectionChanged(const QList<int>&, const QList<int>&)));
		connect(mSelectionModel, SIGNAL(currentChanged(int, int)), this, SLOT(currentThumbChanged(int, int)));
	}
	else {
		qDebug() << "MatchTileView::setSelectionModel: model was NULL";
	}
}

MatchSelectionModel *MatchTileView::selectionModel() const {
	return mSelectionModel;
}

QList<QAction *> MatchTileView::actions() const {
	return mActions;
}

QList<QAction *> MatchTileView::toolbarOnlyActions() const {
	return mToolbarOnlyActions;
}

QList<QWidget *> MatchTileView::statusBarWidgets() const {
	return mStatusBarWidgets;
}

void MatchTileView::createActions() {
	mActions << new QAction(QIcon(":/rcc/fatcow/32x32/sort_ascending.png"), tr("Sort ascending"), this);
	mActions.last()->setStatusTip(tr("Sort matches ascending"));
	connect(mActions.last(), SIGNAL(triggered()), this, SLOT(sortAscending()));

	mActions << new QAction(QIcon(":/rcc/fatcow/32x32/sort_descending.png"), tr("Sort descending"), this);
	mActions.last()->setStatusTip(tr("Sort matches descending"));
	connect(mActions.last(), SIGNAL(triggered()), this, SLOT(sortDescending()));

	mActions << new QAction(tr("Separator"), this);
	mActions.last()->setSeparator(true);

	mActions << new QAction(QIcon(":/rcc/fatcow/32x32/google_custom_search.png"), tr("Visible statuses"), this);
	mActions.last()->setStatusTip(tr("Select the statuses that should be visible"));
	connect(mActions.last(), SIGNAL(triggered()), this, SLOT(filterStatuses()));

	// filter widget that will appear in the toolbar

	// we put a the real widget in a layout in another widget just so we can make it appear on the right side... for esthetic reasons
	QWidgetAction *wb = new QWidgetAction(this);
	QWidget *containerWidget = new QWidget;
	QToolButton *lookingGlass = new QToolButton(); {
		lookingGlass->setAutoRaise(true);
		lookingGlass->setIcon(QIcon(":/rcc/fatcow/32x32/zoom.png"));
		lookingGlass->setToolTip(tr("Will filter matches based on the text to the right. You can use wildcards. <br /><br />'*' matches any sequence of characters and '?' matches one character. <br /><br /><b>Example</b>: <em>WDC1_?1*1</em>"));
		connect(lookingGlass, SIGNAL(clicked()), this, SLOT(filter()));
		// if the size of the toolbutton is unsatisfactory check this out: http://doc.qt.nokia.com/latest/qstyle.html#PixelMetric-enum
	}
	QBoxLayout *containerLayout = new QHBoxLayout(containerWidget);
	mFilterEdit = new QLineEdit();
	connect(mFilterEdit, SIGNAL(returnPressed()), this, SLOT(filter()));

	QSizePolicy sizePolicy = mFilterEdit->sizePolicy();
	sizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
	mFilterEdit->setSizePolicy(sizePolicy);

	containerLayout->addStretch(30);
	containerLayout->addWidget(lookingGlass, Qt::AlignRight);
	containerLayout->addWidget(mFilterEdit, Qt::AlignRight);

	wb->setDefaultWidget(containerWidget);
	//wb->setDefaultWidget(mFilterEdit);
	mToolbarOnlyActions << wb;

	mCopyAction = new QAction(QIcon(":/rcc/fatcow/32x32/page_copy.png"), tr("Copy"), this);
	mCopyAction->setShortcuts(QKeySequence::Copy);
	mCopyAction->setStatusTip(tr("Copy this match to the clipboard"));
	connect(mCopyAction, SIGNAL(triggered()), this, SLOT(copyCurrent()));

	mCommentAction = new QAction(QIcon(":/rcc/fatcow/32x32/comment_add.png"), tr("Comment"), this);
	mCommentAction->setStatusTip(tr("Add a comment to the match"));
	connect(mCommentAction, SIGNAL(triggered()), this, SLOT(comment()));

	mFindDuplicatesAction = new QAction(QIcon(":/rcc/fatcow/32x32/shape_ungroup.png"), tr("Find duplicate matches"), this);
	mFindDuplicatesAction->setStatusTip(tr("Display all duplicates of this match (i.e.: every match that consists of the same fragments)"));
	connect(mFindDuplicatesAction, SIGNAL(triggered()), this, SLOT(comment()));

	mFindConflictingAction = new QAction(QIcon(":/rcc/fatcow/32x32/sql_join_inner.png"), tr("Find conflicting matches"), this);
	mFindConflictingAction->setStatusTip(tr("Display matches that conflict with this one (i.e.: all matches that have some overlap with this one so that both can't be correct)"));
	connect(mFindConflictingAction, SIGNAL(triggered()), this, SLOT(comment()));

	mFindNonconflictingAction = new QAction(QIcon(":/rcc/fatcow/32x32/sql_join_outer_exclude.png"), tr("Find non-conflicting matches"), this);
	mFindNonconflictingAction->setStatusTip(tr("Display matches that do not conflict with this one (i.e.: all matches that have have a fragment in common but are not mutually exclusive)"));
	connect(mFindNonconflictingAction, SIGNAL(triggered()), this, SLOT(comment()));
}

void MatchTileView::createStatusWidgets() {
	mStatusBarLabel = new QLabel;
	mStatusBarWidgets << mStatusBarLabel;
}

void MatchTileView::statusMenuTriggered(QAction *action) {
	int status = IMatchModel::STATUS_STRINGS.indexOf(action->text());

	if (status != -1) {
		setStatus((IMatchModel::Status) status);
	}
}

void MatchTileView::sortAscending() {
	sort(Qt::AscendingOrder);
}

void MatchTileView::sortDescending() {
	sort(Qt::DescendingOrder);
}

void MatchTileView::sort(Qt::SortOrder order) {
	bool ok = false;

	// QInputDialog::getItem requires an actual QStringList and not a set, unfortunately
	QStringList fieldList = QStringList::fromSet(mModel->fieldList());

	if (!fieldList.isEmpty()) {
		QString field = QInputDialog::getItem(this, "Sort matches", "Choose an attribute to sort by", fieldList, 0, false, &ok);

		if (ok && !field.isEmpty()) {
			mModel->sort(field, order);
		}
	}
}

void MatchTileView::filter() {
	QString filter = mFilterEdit->text().trimmed();

	mModel->filter(filter);
}

void MatchTileView::filterStatuses() {
	typedef QPair<QString, bool> StringBoolPair;

	QList<StringBoolPair> statuses;

	// this system needs to be redesigned for dynamic statuses anyway so it doesn't have to be too clean
	// this should all be in the model!!!
	assert(IMatchModel::NUM_STATUSES == IMatchModel::STATUS_STRINGS.size());

	for (int i = 0; i < IMatchModel::NUM_STATUSES; ++i) {
		bool activated = false;

		switch (i) {
			case IMatchModel::CONFLICT: activated = s().show_conflicted; break;
			case IMatchModel::MAYBE: activated = s().show_maybe; break;
			case IMatchModel::NO: activated = s().show_rejected; break;
			case IMatchModel::UNKNOWN: activated = s().show_unknown; break;
			case IMatchModel::YES: activated = s().show_confirmed; break;
		}

		statuses << StringBoolPair(IMatchModel::STATUS_STRINGS.at(i), activated);
	}

	ShowStatusDialog dialog(this, statuses);

	int ret = dialog.exec();

	if (!!ret) {
		statuses = dialog.getStatuses();

		QStringList disabled;

		// TODO: this is code that relies on ordering and other things, should replace in time
		foreach (const StringBoolPair& status, statuses) {
			for (int i = 0; i < IMatchModel::NUM_STATUSES; ++i) {
				if (IMatchModel::STATUS_STRINGS.at(i) == status.first) {
					switch ((IMatchModel::Status) i) {
						case IMatchModel::CONFLICT: s().show_conflicted = status.second; break;
						case IMatchModel::MAYBE: s().show_maybe = status.second; break;
						case IMatchModel::NO: s().show_rejected = status.second; break;
						case IMatchModel::UNKNOWN: s().show_unknown = status.second; break;
						case IMatchModel::YES: s().show_confirmed = status.second; break;

						default: qDebug() << "MatchTileView::filterStatuses: unknown status encountered:" << status.first << "| number" << i;
					}

					if (status.second == false) disabled << QString::number(i);
				}
			}
		}

		if (!disabled.isEmpty()) {
			mModel->genericFilter("tileview_statuses", QString("status NOT IN (%1)").arg(disabled.join(",")));
		}
		else {
			// this is equivalent to removing the filter
			mModel->genericFilter("tileview_statuses", QString());
		}
	}
}


void MatchTileView::comment() {
	int current = mSelectionModel->currentIndex();

	if (mModel->isValidIndex(current)) {
		IFragmentConf &match = mModel->get(current);
		QString comment = match.getString("comment", "");

		bool ok;
		comment = QInputDialog::getText(this, tr("Comment"), tr("Insert comment") + ":", QLineEdit::Normal, comment, &ok);

		if (ok) {
			//qDebug() << "MatchTileView::comment: Going to insert comment at (" << modelToViewIndex(current) << "," << current << ")";

			match.setMetaData("comment", comment);

			updateThumbnail(modelToViewIndex(current), current);
		}
	}
	else {
		qDebug() << "MatchTileView::comment: invalid model index" << current;
	}
}

void MatchTileView::updateStatusBar() {
	int lastValidIndex = mNumThumbs - 1;
	while (lastValidIndex >= 0 && s().tindices[lastValidIndex] < 0) {
		--lastValidIndex;
	}

	if (lastValidIndex >= 0) {
		QString message = QString("Browsing %1 (%2) to %3 (%4) of %5")
			.arg(s().cur_pos + 1)
			.arg(mModel->get(s().tindices[0]).getDouble("error"))
			.arg(s().cur_pos + lastValidIndex + 1)
			.arg(mModel->get(s().tindices[lastValidIndex]).getDouble("error"))
			.arg(mModel->size());

		mStatusBarLabel->setText(message);
	}
	else {
		mStatusBarLabel->setText("");
	}
}

void MatchTileView::updateThumbnail(int tidx, int fcidx) {
	if (tidx < 0 || tidx >= mNumThumbs) {
		return;
	}

	s().tindices[tidx] = fcidx;

	if (fcidx < 0 || fcidx >= mModel->size()) {
		mThumbs[tidx]->setThumbnail();
	}
	else {
		const IFragmentConf& match = mModel->get(fcidx);

		QString thumbFile = mThumbDir.absoluteFilePath(thumbName(match));
		mThumbs[tidx]->setThumbnail(thumbFile, (IMatchModel::Status) match.getString("status", "0").toInt());

		if (mSelectionModel->isSelected(fcidx)) {
			mThumbs[tidx]->select();
		}

		QString tooltip = QString("<b>Target</b>: %1<br /><b>Source</b>: %2<br /><b>Error</b>: %3<br /><b>Volume</b>: %4")
				.arg(Database::fragment(match.mFragments[IFragmentConf::TARGET])->id())
				.arg(Database::fragment(match.mFragments[IFragmentConf::SOURCE])->id())
				.arg(match.getString("error", ""))
				.arg(match.getString("volume", ""));

		QString comment = match.getString("comment", QString());

		if (!comment.isEmpty()) {
			tooltip += "<br /><b>Comment</b>: " + comment;
		}

		mThumbs[tidx]->setToolTip(tooltip);
	}
}

void MatchTileView::setStatus(IMatchModel::Status status) {
	foreach (int modelIndex, mSelectionModel->selectedIndexes()) {
		IFragmentConf &c = mModel->get(modelIndex);

		int currentStatus = c.getInt("status", 0);

		if ((IMatchModel::Status) currentStatus != status) {
			c.setMetaData("status", QString::number(status));

			updateThumbnail(modelToViewIndex(modelIndex), modelIndex);

			// set a flag if any change to/from "YES" and "MAYBE" was made
		}
	}

	/*
	qDebug() << FragmentRef(c.mFragments[FragmentConf::TARGET]).id() <<
			FragmentRef(c.mFragments[FragmentConf::SOURCE]).id() <<
			c.getDouble("error", -1);

	int cur_status = c.getString("status", "0").toInt();
	if (cur_status == status) return;

	int old_status = c.getString("status", "0").toInt();

	undo_list.push_back(std::vector< std::pair<int, int> >());
	std::vector< std::pair<int, int> > &u = undo_list.back();

	c.setMetaData("status", QString::number(status));
	// fprintf(stderr, "pushing %d %d\n", s().tindices[idx], cur_status);
	u.push_back(std::pair<int, int>(s().tindices[idx], cur_status));

	if (old_status == YES || old_status == MAYBE || status == YES || status == MAYBE) {
		recomputeConflicts();
	}

	updateThumbnailStatuses();
	*/
}

QString MatchTileView::thumbName(const IFragmentConf &conf) const {
	FragmentRef target(conf.mFragments[IFragmentConf::TARGET]);
	FragmentRef source(conf.mFragments[IFragmentConf::SOURCE]);

	if (target.isNil() || source.isNil()) {
		return QString();
	}

	/*
	if (target.id() > source.id()) {
		conf.swapSourceAndTarget();
		std::swap(source, target);
	}
	*/

	return QString("%3_%1_%2_%4.jpg").arg(target.id(), source.id(),
			QString::number(conf.getDouble("error"), 'f', 4),
			QString::number(conf.getDouble("volume"), 'f', 4));
}

void MatchTileView::clicked(int idx, QMouseEvent *event) {
	if (s().tindices[idx] != mSelectionModel->currentIndex()) {
		if (event->buttons() == Qt::LeftButton && event->modifiers() == Qt::ControlModifier) {
			mSelectionModel->select(s().tindices[idx], QItemSelectionModel::Select | QItemSelectionModel::Current);
		}
		else {
			mSelectionModel->select(s().tindices[idx], QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
		}
	}

	switch (event->buttons()) {
		case Qt::LeftButton:
		{
			// select
			int fcidx = s().tindices[idx];

			if (event->modifiers() == Qt::ShiftModifier) {
				qDebug() << "MatchTileView::clicked: copying " << thumbName(mModel->get(fcidx));

				copyCurrent();
			}
			/*
			else if (event->modifiers() == Qt::ControlModifier) {
				comment();
			}
			*/
		}
		break;

		case Qt::MidButton:
		{
			if (!mSelectionModel->isEmpty()) {
				setStatus(IMatchModel::YES);
			}
		}
		break;

		case Qt::RightButton:
		{
			if (!mSelectionModel->isEmpty()) {
				mStatusMenu->popup(QCursor::pos());
			}
		}
		break;
	}
}

void MatchTileView::doubleClicked(int idx, QMouseEvent *) {
#ifdef WITH_DETAILVIEW
	const IFragmentConf &c = mModel->get(s().tindices[idx]);

	FragmentRef target(c.mFragments[IFragmentConf::TARGET]);
	FragmentRef source(c.mFragments[IFragmentConf::SOURCE]);

	mTabletopModel.fragmentPlace(target.id(), XF());
	mTabletopModel.fragmentPlace(source.id(), c.mXF);

	mDetailScene.init(&mTabletopModel);
	mDetailView.show(); // make it visible
#endif
}

void MatchTileView::copyCurrent() {
	qDebug() << "Copy triggered on thumbnail";

	TabletopModel pairModel;
	int idx = 5;

	const IFragmentConf &c = mModel->get(s().tindices[idx]);

	FragmentRef target(c.mFragments[IFragmentConf::TARGET]);
	FragmentRef source(c.mFragments[IFragmentConf::SOURCE]);

	pairModel.fragmentPlace(target.id(), XF());
	pairModel.fragmentPlace(source.id(), c.mXF);
	pairModel.group(QSet<const Placement *>()
		<< pairModel.placedFragment(target.id())
		<< pairModel.placedFragment(source.id()));

	QString xml = writeToString(pairModel);
	QApplication::clipboard()->setText(xml);
}

void MatchTileView::copySelection() {
	// construct clean TabletopModel which will contain all pieces currently in the matchbrowser
	TabletopModel model;

	// figure out which fragments should be grouped together
	QHash<int, int> groups;
	int next_group = 0;

	foreach (int index, mSelectionModel->selectedIndexes()) {
		const IFragmentConf &c = mModel->get(index);

		int tgt = c.mFragments[IFragmentConf::TARGET];
		int src = c.mFragments[IFragmentConf::SOURCE];

		FragmentRef target(tgt);
		FragmentRef source(src);

		if (groups.contains(tgt) && groups.contains(src)) {
			// both tgt and src fragments have been seen already
			int tidx = groups[tgt];
			int sidx = groups[src];

			if (tidx == sidx) {
				continue; // and they're already in the same group
			}

			qDebug() << "Computing motion based on" << FragmentRef(source) << FragmentRef(target);
			XF dxf = model.placedFragment(target)->accumXF() * c.mXF;
			dxf = dxf * inv(model.placedFragment(source)->accumXF());

			// merge the groups containing src and tgt
			foreach (int key, groups.keys()) {
				if (groups[key] == sidx) {
					groups[key] = tidx;
					const Placement *p = model.placedFragment(FragmentRef(key));
					assert(p);
					qDebug() << "Moving" << FragmentRef(key);
					XF xf = p->accumXF();
					std::cerr << xf;
					model.setXF(p, p->xf() * inv(xf) * dxf * xf);
				}
			}
		}
		else if (groups.contains(tgt)) {
			// src should go in the same group as tgt
			groups[src] = groups[tgt];
			model.fragmentPlace(source, model.placedFragment(target)->accumXF() * c.mXF);
		}
		else if (groups.contains(src)) {
			// tgt should go in the same group as src
			groups[tgt] = groups[src];
			model.fragmentPlace(target, model.placedFragment(source)->accumXF() * inv(c.mXF));
		}
		else {
			// src and tgt should go in a new group
			groups[src] = next_group++;
			groups[tgt] = groups[src];

			model.fragmentPlace(target, XF());
			model.fragmentPlace(source, c.mXF);
		}
	}

	// loop over all matches (in the fc vector) and add them to the model in the appropriate group
	QVector< QSet<const Placement *> > placedFragments(next_group);

	foreach (int index, mSelectionModel->selectedIndexes()) {
		const IFragmentConf &c = mModel->get(index);

		const FragmentRef target(c.mFragments[IFragmentConf::TARGET]);
		const FragmentRef source(c.mFragments[IFragmentConf::SOURCE]);

		const PlacedFragment *ptgt = model.placedFragment(target.id());
		const PlacedFragment *psrc = model.placedFragment(source.id());

		if (!ptgt && !psrc) {
			qDebug() << "Placing both" << target.id() << "and" << source.id();
			model.fragmentPlace(target.id(), XF());
			model.fragmentPlace(source.id(), c.mXF);
		}
		else if (!ptgt) {
			model.fragmentPlace(target.id(), psrc->accumXF() * inv(c.mXF));
		}
		else if (!psrc) {
			qDebug() << "Placing source" << source.id() << "next to" << target.id();
			model.fragmentPlace(source.id(), ptgt->accumXF() * c.mXF);
		} // else both fragments are already placed

		// add pair annotation
		QString status  = IMatchModel::STATUS_STRINGS[c.getString("status", "0").toInt()];
		QString comment = c.getString("comment", QString());

		PairAnnotation pa(source.id(), target.id(), status, comment);
		model.addPairAnnotation(pa);

		// const PlacedFragment *placedTarget = model.placedFragment(target.id());
		// const PlacedFragment *placedSource = model.placedFragment(source.id());

		// qDebug() << "matchBrowser::copyAll: placedTarget was" << ((placedTarget == NULL) ? "NULL" : "OK") << ". Match id =" << c.getString("id") << ", Fragment id =" << target.id();
		// qDebug() << "matchBrowser::copyAll: placedSource was" << ((placedSource == NULL) ? "NULL" : "OK") << ". Match id =" << c.getString("id") << ", Fragment id =" << source.id();

		assert(groups.contains(c.mFragments[IFragmentConf::TARGET]));
		assert(groups.contains(c.mFragments[IFragmentConf::SOURCE]));

		qDebug() << c.mFragments[IFragmentConf::TARGET] << groups[c.mFragments[IFragmentConf::TARGET]];
		qDebug() << c.mFragments[IFragmentConf::SOURCE] << groups[c.mFragments[IFragmentConf::SOURCE]];

		int g = groups[c.mFragments[IFragmentConf::TARGET]];

		assert(g == groups[c.mFragments[IFragmentConf::SOURCE]]);
		assert(g < placedFragments.size());

		placedFragments[g]
			<< model.placedFragment(target.id())
			<< model.placedFragment(source.id());
	}

	foreach (const QSet<const Placement *>& group, placedFragments) {
		if (!group.isEmpty()) {
			model.group(group);
		}
	}

	// write current TabletopModel to string and copy to clipboard
	QString xml = writeToString(model);
	QApplication::clipboard()->setText(xml);
}

void MatchTileView::modelChanged() {
	qDebug() << "MatchTileView::modelChanged: called";

	mFilterEdit->setText(mModel->getFilter());

	s().cur_pos = 0;
	refresh();
}

void MatchTileView::modelOrderChanged() {
	qDebug() << "MatchTileView::modelOrderChanged: called";

	s().cur_pos = 0;
	refresh();
}

void MatchTileView::selectionChanged(const QList<int>& selected, const QList<int>& deselected) {
	qDebug() << "MatchTileView::selectionChanged: selection changed, selected:" << selected.size() << "|| deselected:" << deselected.size();

	QList<int> viewIndices = modelToViewIndex(selected);

	foreach (int viewIndex, viewIndices) {
		mThumbs[viewIndex]->select();
	}

	viewIndices = modelToViewIndex(deselected);

	foreach (int viewIndex, viewIndices) {
		mThumbs[viewIndex]->unselect();
	}
}

void MatchTileView::currentThumbChanged(int current, int previous) {
	qDebug() << "MatchTileView::currentThumbChanged: current thumb changed to" << current << "( was" << previous << ")";
}

/*
void MatchTileView::resizeEvent(QResizeEvent *event) {
	refresh();
}
*/

void MatchTileView::keyPressEvent(QKeyEvent *event) {
	switch (event->key()) {
		case Qt::Key_A:
		{
			if (event->modifiers() & Qt::ControlModifier) {
				QList<int> list;

				foreach (int modelIndex, s().tindices) {
					list << modelIndex;
				}

				mSelectionModel->select(list, QItemSelectionModel::ClearAndSelect);
			}
		}
		break;

		case Qt::Key_C:
		{
			if (event->modifiers() & Qt::ControlModifier) {
				copySelection();
			}
		}
		break;

		case Qt::Key_S:
		{
			static bool order = true;

			mModel->sort("error", order ? Qt::AscendingOrder : Qt::DescendingOrder);

			order = !order;
		}
		break;

		case Qt::Key_N:
		{
			// set all visible UNKNOWN's to NO
		    bool changed = false;

		    //undo_list.push_back(std::vector< std::pair<int, int> >());
		    for (int i = 0; i < mNumThumbs; ++i) {
		    	const IFragmentConf& c = mModel->get(s().tindices[i]);

		    	int status = c.getString("status", "0").toInt();

		    	if (status == IMatchModel::UNKNOWN) {
		    		c.setMetaData("status", QString::number(IMatchModel::NO));

		    		//undo_list.back().push_back(std::pair<int, int>(s().tindices[i], status));

		    		changed = true;
		    	}
		    }
		    if (changed) {
		    	refresh();
		    }
		    else {
		    	//undo_list.pop_back();
		    }
		}
		break;

		case Qt::Key_PageDown:
		case Qt::Key_Space:
		case Qt::Key_Down:
		{
			int multiplier = 1; // one screen

			if (event->modifiers() == Qt::ShiftModifier) {
				multiplier = 10; // 10 screens
			}
			else if (event->modifiers() == Qt::ControlModifier) {
				multiplier = 100; // 100 screens
			}

			scroll(multiplier * mNumThumbs);
		}
		break;

		case Qt::Key_PageUp:
		case Qt::Key_Up:
		{
			int multiplier = -1; // one screen

			if (event->modifiers() == Qt::ShiftModifier) {
				multiplier = 10; // 10 screens
			}
			else if (event->modifiers() == Qt::ControlModifier) {
				multiplier = 100; // 100 screens
			}

			scroll(multiplier * mNumThumbs);
		}
		break;

		default:
			QScrollArea::keyPressEvent(event);
	}
}

void MatchTileView::scroll(int amount) {
	int max = mModel->size();

	int new_pos = qMax(s().cur_pos + amount, 0);

	if (new_pos == s().cur_pos) {
		qDebug() << "new_pos" << new_pos << "| cur_pos" << s().cur_pos;

		return;
	}

	// update the current state
	s().total = max;
	new_pos = qMax(0, qMin(new_pos, max - mNumThumbs));

	/*
	if (new_pos == s().cur_pos) {
		return;
	}
	*/

	s().cur_pos = new_pos;

	// reload thumbnails
	for (int i = 0; i < mNumThumbs; ++i) {
		// if (i + new_pos) doesn't fit in valid.size(), load an empty thumbnail (-1)
		updateThumbnail(i, (max >= i + new_pos) ? i + new_pos : -1);
	}

	updateStatusBar();
}

void MatchTileView::refresh() {
	int max = mModel->size();

	// update the current state
	int new_pos = qMax(0, qMin(s().cur_pos, max - mNumThumbs));
	s().total = max;
	s().cur_pos = new_pos;

	// reload thumbnails
	for (int i = 0; i < mNumThumbs; ++i) {
		// if (i + new_pos) doesn't fit in valid.size(), load an empty thumbnail (-1)
		updateThumbnail(i, (max > i + new_pos) ? i + new_pos : -1);
	}

	updateStatusBar();
}

void MatchTileView::currentValidIndices(QVector<int>& valid) {
	// it's possible the vector still contains data, better to make sure and clear it
	valid.clear();

	if (s().conflict_index < 0) {
		// this means we're not looking for all conflicts to s().conflict_index, business as usual
	}
	else {
		// in this case (s().conflict_index >= 0), load everything that conflicts with s().conflict_index, and the element itself

		/*
		QStringList conflicts;

		conflicts << fc[s().conflict_index].getString("id").trimmed(); // select the element
		conflicts
				<< fc[s().conflict_index].getString("conflict", QString()).split(
						" "); // select the elements it conflicts with

		//qDebug() << "matchBrowser::currentValidIndices: conflict_index =" << s().conflict_index << ">= 0:"
		//    << "id of conflict search fragment:" << fc[s().conflict_index].getString("id")
		//    << "conflicts with:" << conflicts;

		for (int i = conflicts.size() - 1; i >= 0; i--) {
			// remove all id's that are not legal

			bool isNum;
			int id = conflicts[i].toInt(&isNum);
			if (isNum && idmap.contains(id)) {
				assert(idmap[i] < fc.size());

				valid.push_back(idmap[id]);

				//qDebug() << "matchBrowser::currentValidIndices: pushed back idmap[" << id << "] =" << idmap[id];
			}
		}
		*/
	}
}

int MatchTileView::modelToViewIndex(int modelIndex) const {
	return s().tindices.indexOf(modelIndex);
}

/**
 * Will silently drop indices to are not currently available as view indexes
 */
QList<int> MatchTileView::modelToViewIndex(const QList<int>& modelIndexes) const {
	QList<int> list;

	foreach (int index, modelIndexes) {
		int viewIndex = modelToViewIndex(index);

		if (viewIndex != -1) {
			list << viewIndex;
		}
	}

	return list;
}
