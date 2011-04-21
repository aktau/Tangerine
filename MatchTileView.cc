#include "MatchTileView.h"

#include <QMessageBox>
#include <QKeyEvent>
#include <QDebug>
#include <QPixmap>

#include "Database.h"
#include "Fragment.h"
#include "FragmentRef.h"

using namespace thera;

#define THUMB_WIDTH 722
#define THUMB_HEIGHT 466
#define THUMB_GUTTER 10

MatchTileView::MatchTileView(const QDir& thumbDir, QWidget *parent, int rows, int columns, float scale) : QScrollArea(parent), mThumbDir(thumbDir), mModel(NULL), mScale(scale) {
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

	setModel(&MatchModel::EMPTY);
}

MatchTileView::~MatchTileView() {
}

void MatchTileView::setModel(MatchModel *model) {
	if (model != NULL) {
		mModel = model;

		connect(mModel, SIGNAL(modelChanged()), this, SLOT(modelChanged()));

		modelChanged();
	}
	else {
		qDebug() << "MatchTileView::setModel: Invalid model";
	}
}

void MatchTileView::updateThumbnail(int tidx, int fcidx) {
	s().tindices[tidx] = fcidx;

	if (fcidx < 0 || fcidx >= mModel->size()) {
		qDebug() << "Updating [INVALID] thumbnail" << tidx << "to model index" << fcidx;

		qDebug() << "branch 1" << fcidx << "| mModel->size()" << mModel->size();

		QPixmap p(THUMB_WIDTH * mScale, THUMB_HEIGHT * mScale);
		p.fill(Qt::blue);

		mThumbs[tidx]->setPixmap(p);
		mThumbs[tidx]->setToolTip(QString());
	}
	else {
		IFragmentConf& match = mModel->getMatch(fcidx);

		QString thumbFile = mThumbDir.absoluteFilePath(thumbName(match));
		QPixmap p = QPixmap(thumbFile).scaledToWidth(THUMB_WIDTH * mScale, Qt::SmoothTransformation);

		if (p.isNull()) {
			p = QPixmap(THUMB_WIDTH * mScale, THUMB_HEIGHT * mScale);
			p.fill(Qt::lightGray);
		}

		mThumbs[tidx]->setPixmap(p);

		QString tooltip = QString("<b>Target</b>: %1\n<b>Source</b>: %2\n<b>Error</b>: %3\n<b>Volume</b>: %4\n")
				.arg(Database::fragment(match.mFragments[IFragmentConf::TARGET])->id())
				.arg(Database::fragment(match.mFragments[IFragmentConf::SOURCE])->id())
				.arg(match.getString("error", ""))
				.arg(match.getString("volume", ""));

		QString comment = match.getString("comment", "");

		if (!comment.isEmpty()) {
			tooltip += "\n<b>Comment</b>: " + comment;
		}

		qDebug() << "Updating [VALID] thumbnail" << tidx << "to model index" << fcidx << "| thumb = " << thumbFile;

		mThumbs[tidx]->setToolTip(tooltip);
	}

	//updateThumbnailStatus(tidx);
}

QString MatchTileView::thumbName(IFragmentConf &conf) {
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
	Q_UNUSED(event);

	qDebug() << "Clicked!" << idx;
}

void MatchTileView::doubleClicked(int idx, QMouseEvent *event) {
	Q_UNUSED(event);

	qDebug() << "Double Clicked!" << idx;
}

void MatchTileView::modelChanged() {
	qDebug() << "Model changed!";

	s().cur_pos = 0;
	refresh();
}

/*
void MatchTileView::resizeEvent(QResizeEvent *event) {
	refresh();
}
*/

void MatchTileView::keyPressEvent(QKeyEvent *event) {
	switch (event->key()) {
		case Qt::Key_N:
		{
			QMessageBox::about(
				this,
				tr("About Tangerine"),
				tr("<b>Tangerine</b> is a next-generation proof of concept GUI for the <b>Thera project</b>. It intends to aid the user in finding and confirming fragment matches.")
			);
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
	int new_pos = qMax(s().cur_pos + amount, 0);

	if (new_pos == s().cur_pos) {
		qDebug() << "new_pos" << new_pos << "| cur_pos" << s().cur_pos;

		return;
	}

	qDebug() << "Filter =" << s().filter << "isEmpty:" << s().filter.isEmpty();

	QVector<int> valid;
	currentValidIndices(valid); // fill the vector with all currently valid indices (matching filter, etc.)

	// update the current state
	s().total = (int) valid.size();
	new_pos = qMax(0, qMin(new_pos, (int) valid.size() - mNumThumbs));

	/*
	if (new_pos == s().cur_pos) {
		return;
	}
	*/

	s().cur_pos = new_pos;

	// reload thumbnails
	for (int i = 0; i < mNumThumbs; ++i) {
		// if (i + new_pos) doesn't fit in valid.size(), load an empty thumbnail (-1)
		updateThumbnail(i, (valid.size() > i + new_pos) ? valid[i + new_pos] : -1);
	}

	//updateStatusBar();
}

void MatchTileView::refresh() {
	QVector<int> valid;
	currentValidIndices(valid); // fill the vector with all currently valid indices (matching filter, etc.)

	// update the current state
	int new_pos = qMax(0, qMin(s().cur_pos, (int) valid.size() - mNumThumbs));
	s().total = valid.size();
	s().cur_pos = new_pos;

	// reload thumbnails
	for (int i = 0; i < mNumThumbs; ++i) {
		// if (i + new_pos) doesn't fit in valid.size(), load an empty thumbnail (-1)
		updateThumbnail(i, (valid.size() > i + new_pos) ? valid[i + new_pos] : -1);
	}
}


void MatchTileView::currentValidIndices(QVector<int>& valid) {
	// it's possible the vector still contains data, better to make sure and clear it
	valid.clear();

	if (s().conflict_index < 0) {
		// this means we're not looking for all conflicts to s().conflict_index, business as usual
		for (int i = 0, ii = mModel->size(); i < ii; ++i) {
			//qDebug() << "index = " << i << "| size =" << ii;

			IFragmentConf& match = mModel->getMatch(i);

			int status = match.getString("status", "0").toInt();

			if (!s().show_unknown && status == UNKNOWN)
				continue;
			if (!s().show_rejected && status == NO)
				continue;
			if (!s().show_conflicted && status == CONFLICT)
				continue;
			if (!s().show_maybe && status == MAYBE)
				continue;

			//qDebug("IFragmentConf::TARGET[%d] = %d", IFragmentConf::TARGET, match.mFragments[IFragmentConf::TARGET]);
			//qDebug() << "ref = " << FragmentRef(match.mFragments[IFragmentConf::TARGET]);

			QString tid = FragmentRef(match.mFragments[IFragmentConf::TARGET]).id(); // target id
			QString sid = FragmentRef(match.mFragments[IFragmentConf::SOURCE]).id(); // source id

			QRegExp filter(s().filter, Qt::CaseSensitive, QRegExp::Wildcard);
			QString text = tid + "<>" + sid;

			if (!s().filter.isEmpty() && !text.contains(filter)) {
				// qDebug() << "Filtered" << text;
				continue;
			}

			if (!(s().show_conflicted || s().show_rejected || s().show_unknown)) {
				// fprintf(stderr, "%d %s\n", status, qPrintable(thumbName(fc[i])));
			}

			valid.append(i);
		}
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
