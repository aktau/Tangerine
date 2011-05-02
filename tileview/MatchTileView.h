#ifndef MATCHTILEVIEW_H_
#define MATCHTILEVIEW_H_

#include <QScrollArea>
#include <QFrame>
#include <QList>
#include <QVector>
#include <QList>
#include <QLabel>
#include <QDir>
#include <QAction>

#include "IMatchModel.h"
#include "IFragmentConf.h"

class ThumbLabel : public QLabel {
		Q_OBJECT;

	public:
		ThumbLabel(int i, QWidget *parent = NULL) : QLabel(parent), idx(i) {}

	signals:
		void clicked(int i, QMouseEvent *event);
		void doubleClicked(int i, QMouseEvent *event);

	protected:
		virtual void mousePressEvent(QMouseEvent *event) { emit clicked(idx, event); }
		virtual void mouseDoubleClickEvent(QMouseEvent *event) { emit doubleClicked(idx, event); }

	private:
		const int idx;
};

class MatchTileView : public QScrollArea {
		Q_OBJECT

	public:
		enum { UNKNOWN, YES, MAYBE, NO, CONFLICT, NUM_STATUSES };

	public:
		MatchTileView(const QDir& thumbDir, QWidget *parent = NULL, int rows = 4, int columns = 5, float scale = 0.5f);
		virtual ~MatchTileView();

		virtual void setModel(IMatchModel *model);

		QList<QAction *> actions() const;

	public slots:
	    void clicked(int idx, QMouseEvent *event);
	    void doubleClicked(int idx, QMouseEvent *event);
	    void modelChanged();
	    void modelOrderChanged();

	    void sortAscending();
	    void sortDescending();
	    void filter();

	protected:
	    //virtual void resizeEvent(QResizeEvent *event);
		virtual void keyPressEvent(QKeyEvent *event);

	private:
		void createActions();

		void updateThumbnail(int tidx, int fcidx);
		QString thumbName(thera::IFragmentConf &conf);
		void scroll(int amount);
		void refresh();
		void sort(Qt::SortOrder order);
		void currentValidIndices(QVector<int>& valid);

	private:
		QList<QAction *> mActions;

		QDir mThumbDir;

		QFrame *mFrame;
		QVector<QLabel *> mThumbs;

		IMatchModel *mModel;

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
};

#endif /* MATCHTILEVIEW_H_ */
