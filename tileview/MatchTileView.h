#ifndef MATCHTILEVIEW_H_
#define MATCHTILEVIEW_H_

#include <QScrollArea>
#include <QFrame>
#include <QList>
#include <QAction>
#include <QMenu>
#include <QLineEdit>
#include <QVector>
#include <QList>

#include "IMatchModel.h"
#include "IFragmentConf.h"
#include "MatchSelectionModel.h"
#include "TabletopModel.h"
#include "ThumbLabel.h"

#ifdef WITH_DETAILVIEW
#	include "DetailView.h"
#endif

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
	    void copySelection();

	    void modelChanged();
	    void modelOrderChanged();

	    void selectionChanged(const QList<int>& selected, const QList<int>& deselected);
	    void currentThumbChanged(int current, int previous);

	    void statusMenuTriggered(QAction *action);
	    void sortAscending();
	    void sortDescending();
	    void filter();
	    void filterStatuses();
	    void comment();

	    void listDuplicates();
	    void findDuplicates();
	    void markDuplicates();
	    void markAsMaster();

	protected:
	    //virtual void resizeEvent(QResizeEvent *event);
		virtual void keyPressEvent(QKeyEvent *event);

	private:
		void createActions();
		void createStatusWidgets();

		void goBack();
		void scroll(int amount);
		void refresh();

		void updateStatusBar();
		void updateThumbnail(int tidx, int fcidx);
		void setStatus(IMatchModel::Status status);
		void currentValidIndices(QVector<int>& valid);

		void sort(Qt::SortOrder order);

		int modelToViewIndex(int modelIndex) const;
		QList<int> modelToViewIndex(const QList<int>& modelIndexes) const;

		QString thumbName(const thera::IFragmentConf &conf) const;

	private:
		QList<QAction *> mActions;
		QList<QAction *> mToolbarOnlyActions;
		QList<QWidget *> mStatusBarWidgets;
		QLabel *mStatusBarLabel;
		QLineEdit *mFilterEdit;

		QMenu *mStatusMenu;
		QMenu *mDuplicatesMenu;
		QAction *mCopyAction;
		QAction *mCommentAction;
		QAction *mFindDuplicatesAction, *mListDuplicatesAction, *mMarkAsDuplicateAction, *mMarkAsMasterAction;
		QAction *mFindConflictingAction;
		QAction *mFindNonconflictingAction;
		QList<QAction *> mStatusMenuActions;

		QDir mThumbDir;

		QFrame *mFrame;
		QVector<ThumbLabel *> mThumbs;

		IMatchModel *mModel;
		MatchSelectionModel *mSelectionModel;

		int mNumThumbs;
		float mScale;

		// Detailed view
		DetailView mDetailView;
		DetailScene mDetailScene;

		thera::TabletopModel mTabletopModel;

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
			bool show_rejected, show_conflicted, show_maybe, show_unknown, show_confirmed;

			int cur_pos, total; // the current position in the list of proposals

			QVector<int> tindices;

			// duplicate manipulation
			bool isSelectingMaster;
			QList<int> duplicateCandidates;

			State(int nt) :
				filter(""),
				conflict_index(-1),
				show_rejected(true),
				show_conflicted(true),
				show_maybe(true),
				show_unknown(true),
				show_confirmed(true),
				cur_pos(0),
				isSelectingMaster(false) {
				tindices.resize(nt);
			}
		};

		QList<State> mStates;

		State& s() { return mStates.back(); }
		const State& s() const { return mStates.back(); }
};

#endif /* MATCHTILEVIEW_H_ */
