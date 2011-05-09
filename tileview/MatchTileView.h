#ifndef MATCHTILEVIEW_H_
#define MATCHTILEVIEW_H_

#include <QScrollArea>
#include <QFrame>
#include <QList>
#include <QLabel>
#include <QDir>
#include <QAction>
#include <QMenu>
#include <QLineEdit>
#include <QVector>
#include <QList>
#include <QPainter>
#include <QPixmapCache>

#include "IMatchModel.h"
#include "IFragmentConf.h"
#include "MatchSelectionModel.h"

class ThumbLabel : public QLabel {
		Q_OBJECT;

	public:
		ThumbLabel(int i, QWidget *parent = NULL) : QLabel(parent), idx(i), mSelected(false) {
			// ensure 100 MB of cache
			QPixmapCache::setCacheLimit(102400);
		}

		void setThumbnail(const QString& file = QString()) {
			const bool exists = QFile::exists(file);

			if (file.isEmpty()) {
				mSource = ".empty";
			}
			else {
				mSource = exists ? file : ".invalid";
			}

			QPixmap p;

			if (!QPixmapCache::find(mSource, &p)) {
				p = (file.isEmpty() || !exists) ?  QPixmap(width(), height()) : QPixmap(file);

				if (file.isEmpty()) {
					p.fill(Qt::black);
				}
				else if (!exists) {
					p.fill(Qt::lightGray);
				}
				else {
					p = p.scaledToWidth(width(), Qt::SmoothTransformation);
				}

				QPixmapCache::insert(mSource, p);
			}
			/*
			else {
				qDebug() << "cached and found!" << idx << ":" << mSource;
			}
			*/

			mSelected = false;

			setPixmap(p);
		}

		void select() {
			if (!isSelected()) {
				QPixmap p = QPixmap(pixmap()->width(), pixmap()->height());
				QPainter painter(&p);

				painter.drawPixmap(rect(), *pixmap());
				painter.fillRect(rect(), QColor(255,255,255,50));

				setPixmap(p);

				mSelected = true;
			}
		}

		void unselect() {
			if (isSelected()) {
				setThumbnail(mSource);

				mSelected = false;
			}
		}

		bool isSelected() const {
			return mSelected;
		}

	signals:
		void clicked(int i, QMouseEvent *event);
		void doubleClicked(int i, QMouseEvent *event);

	protected:
		virtual void mousePressEvent(QMouseEvent *event) { emit clicked(idx, event); }
		virtual void mouseDoubleClickEvent(QMouseEvent *event) { emit doubleClicked(idx, event); }

	public:
		const int idx;

	private:
		bool mSelected;

		QString mSource;
};

class MatchTileView : public QScrollArea {
		Q_OBJECT

	public:
		MatchTileView(const QDir& thumbDir, QWidget *parent = NULL, int rows = 4, int columns = 5, float scale = 0.5f);
		virtual ~MatchTileView();

		virtual void setModel(IMatchModel *model);
		virtual IMatchModel *model() const;

		virtual void setSelectionModel(MatchSelectionModel *model);
		virtual MatchSelectionModel *selectionModel() const;

		QList<QAction *> actions() const;
		QList<QAction *> toolbarOnlyActions() const;
		QList<QWidget *> statusBarWidgets() const;

	public slots:
	    void clicked(int idx, QMouseEvent *event);
	    void doubleClicked(int idx, QMouseEvent *event);
	    void copyCurrent();
	    void copySelection();

	    void modelChanged();
	    void modelOrderChanged();

	    void selectionChanged(const QList<int>& selected, const QList<int>& deselected);
	    void currentThumbChanged(int current, int previous);

	    void sortAscending();
	    void sortDescending();
	    void filter();
	    void filterStatuses();
	    void comment();

	protected:
	    //virtual void resizeEvent(QResizeEvent *event);
		virtual void keyPressEvent(QKeyEvent *event);

	private:
		void createActions();
		void createStatusWidgets();

		void updateStatusBar();
		void updateThumbnail(int tidx, int fcidx);
		QString thumbName(const thera::IFragmentConf &conf) const;
		void scroll(int amount);
		void refresh();
		void sort(Qt::SortOrder order);
		void currentValidIndices(QVector<int>& valid);

		int modelToViewIndex(int modelIndex) const;
		QList<int> modelToViewIndex(const QList<int>& modelIndexes) const;

	private:
		QList<QAction *> mActions;
		QList<QAction *> mToolbarOnlyActions;
		QList<QWidget *> mStatusBarWidgets;
		QLabel *mStatusBarLabel;
		QLineEdit *mFilterEdit;

		QMenu *mStatusMenu;
		QAction *mCopyAction;
		QAction *mCommentAction;
		QList<QAction *> mStatusMenuActions;

		QDir mThumbDir;

		QFrame *mFrame;
		QVector<ThumbLabel *> mThumbs;

		IMatchModel *mModel;
		MatchSelectionModel *mSelectionModel;

		int mNumThumbs;
		float mScale;

	private:
		struct State {
			// either src or tgt should contain filter
			// to be shown
			QString filter;

			// set to -1 for browsing the list, and
			// to a specific index to show conflicting fragments
			int conflict_index;

			// decide whether or not to show explicitly
			// rejected pairs, and those that conflict
			// with approved pairs
			bool show_rejected, show_conflicted, show_maybe, show_unknown;

			int cur_pos, total; // the current position in the list of proposals

			QVector<int> tindices;

			State(int nt) : filter(""), conflict_index(-1), show_rejected(true), show_conflicted(true),show_maybe(true),  show_unknown(true), cur_pos(0) {
				tindices.resize(nt);
			}
		};

		QList<State> mStates;

		State& s() {
			return mStates.back();
		}

		const State& s() const {
			return mStates.back();
		}
};

#endif /* MATCHTILEVIEW_H_ */
