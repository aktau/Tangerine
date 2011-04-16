#ifndef TANGERINE_H_
#define TANGERINE_H_

#include <QMainWindow>
#include <QScrollArea>
#include <QFrame>
#include <QMenu>
#include <QToolbar>
#include <QAction>
#include <QProgressDialog>
#include <QLabel>

#include "SQLDatabase.h"

#define DEV_PHASE "Alpha"
#define MAJ_VERSION 0
#define MIN_VERSION 2

class Tangerine : public QMainWindow {
	Q_OBJECT

	public:
		Tangerine(QWidget *parent = 0);
		virtual ~Tangerine();

	private:
		void setupWindow();
		void createActions();
		void closeDatabase();

	private slots:
		void loadDatabase();
		void saveDatabase();
		void importDatabase();
		void exportDatabase();

		void databaseOpened();
		void databaseClosed();

		void databaseOpStarted(const QString& operation, int steps);
		void databaseOpStepDone(int step);
		void databaseOpEnded();

		void updateStatusBar();

		void about();

	private:
		SQLDatabase *mDb;

		QFrame *mFrame;
		QScrollArea *mScrollArea;
		QProgressDialog *mProgress;

		QLabel *mNumberOfMatchesLabel;

		QMenu *mFileMenu;
		QMenu *mHelpMenu;
		QToolBar *mFileToolbar;

		QAction *mLoadDbAct;
		QAction *mSaveDbAct;
		QAction *mImportXMLAct;
		QAction *mSaveXMLAct;
		QAction *mHelpAboutAct;

		static const int MIN_WIDTH;
		static const int MIN_HEIGHT;

		static const QString MATCH_COUNT_TEXT;
};

#endif /* TANGERINE_H_ */
