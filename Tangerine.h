#ifndef TANGERINE_H_
#define TANGERINE_H_

#include <QMainWindow>
#include <QScrollArea>
#include <QFrame>
#include <QMenu>
#include <QToolbar>
#include <QAction>
#include <QProgressDialog>
#include <QStackedWidget>
#include <QLabel>
#include <QDir>
#include <QList>

#include "SQLDatabase.h"
#include "MatchModel.h"

#ifdef WITH_TILEVIEW
#	include "MatchTileView.h"
#endif
#ifdef WITH_GRAPH
#	include "GraphView.h"
#endif

#define SETTINGS_DB_ROOT_KEY "db/root"

#define DEV_PHASE "Alpha"
#define MAJ_VERSION 0
#define MIN_VERSION 5

class Tangerine : public QMainWindow {
	Q_OBJECT

	public:
		Tangerine(SQLDatabase *db, const QDir& thumbDir, QWidget *parent = 0);
		virtual ~Tangerine();

	private:
		void setupWindow();
		void createActions();
		void closeDatabase();

	private slots:
		void loadFragmentDatabase();
		void matchCountChanged();
		void loadMatchDatabase();
		void saveDatabase();
		void importDatabase();
		void exportDatabase();

		void addAttribute();
		void removeAttribute();

		void normalView();
		void nodeView();

		void about();

		void updateStatusBar();

		void fragmentDatabaseOpened();
		void databaseOpened();
		void databaseClosed();

		void databaseOpStarted(const QString& operation, int steps);
		void databaseOpStepDone(int step);
		void databaseOpEnded();

	private:
		SQLDatabase& mDb;

		MatchModel mModel;
		MatchSelectionModel *mSelectionModel;

		QDir mThumbDir;

		/* GUI ELEMENTS */
		QStackedWidget *mCentralWidget;

#ifdef WITH_TILEVIEW
		MatchTileView *mTileView;
		QMenu *mTileViewMenu;
		QToolBar *mTileViewToolbar;
#endif
#ifdef WITH_GRAPH
		GraphView *mGraphView;
		QMenu *mGraphViewMenu;
		QToolBar *mGraphViewToolbar;
#endif

		QProgressDialog *mProgress;

		QLabel *mNumberOfMatchesLabel;

		QMenu *mFileMenu;
		QMenu *mEditMenu;
		QMenu *mViewMenu;
		QMenu *mHelpMenu;
		QToolBar *mFileToolbar;

		QAction *mLoadFragDbAct;
		QAction *mLoadMatchDbAct;
		QAction *mSaveDbAct;

		QAction *mImportXMLAct;
		QAction *mSaveXMLAct;

		QAction *mAddAttributeAct;
		QAction *mRemoveAttributeAct;

		QActionGroup *mViewGroup;
		QAction *mNormalViewAct;
		QAction *mNodeViewAct;

		QAction *mHelpAboutAct;

		static const int MIN_WIDTH;
		static const int MIN_HEIGHT;

		static const QString MATCH_COUNT_TEXT;
};

#endif /* TANGERINE_H_ */
