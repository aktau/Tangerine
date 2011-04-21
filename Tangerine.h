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

#include "SQLDatabase.h"
#include "MatchModel.h"
#include "MatchTileView.h"

#define SETTINGS_DB_ROOT_KEY "db/root"

#define DEV_PHASE "Alpha"
#define MAJ_VERSION 0
#define MIN_VERSION 2

class Tangerine : public QMainWindow {
	Q_OBJECT

	public:
		Tangerine(SQLDatabase& db, QWidget *parent = 0);
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
		//QList<thera::SQLFragmentConf> mFc;

		QStackedWidget *mCentralWidget;

		//QFrame *mFrame;
		//QScrollArea *mScrollArea;
		MatchTileView *mTileView;

		QProgressDialog *mProgress;

		QLabel *mNumberOfMatchesLabel;

		QMenu *mFileMenu;
		QMenu *mViewMenu;
		QMenu *mHelpMenu;
		QToolBar *mFileToolbar;

		QAction *mLoadFragDbAct;
		QAction *mLoadMatchDbAct;
		QAction *mSaveDbAct;

		QAction *mImportXMLAct;
		QAction *mSaveXMLAct;

		QActionGroup *mViewGroup;
		QAction *mNormalViewAct;
		QAction *mNodeViewAct;

		QAction *mHelpAboutAct;

		static const int MIN_WIDTH;
		static const int MIN_HEIGHT;

		static const QString MATCH_COUNT_TEXT;
};

#endif /* TANGERINE_H_ */
