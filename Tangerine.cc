#include "Tangerine.h"

#include <QtGui>
#include <QDebug>
#include <QProgressBar>

const int Tangerine::MIN_WIDTH = 1024;
const int Tangerine::MIN_HEIGHT = 786;

Tangerine::Tangerine(QWidget *parent) : QMainWindow(parent), mDb(NULL), mProgress(NULL) {
	setWindowIcon(QIcon("tangerine.ico"));
	setToolButtonStyle(Qt::ToolButtonIconOnly);

	setupWindow();

	// the ordering is important, the slots use instances made in setupWindow() et cetera
	mDb = new SQLDatabase();

	connect(mDb, SIGNAL(databaseOpened()), this, SLOT(databaseOpened()));
	connect(mDb, SIGNAL(databaseClosed()), this, SLOT(databaseClosed()));
	connect(mDb, SIGNAL(databaseOpStarted(const QString&, int)), this, SLOT(databaseOpStarted(const QString&, int)));
	connect(mDb, SIGNAL(databaseOpStepDone(int)), this, SLOT(databaseOpStepDone(int)));
	connect(mDb, SIGNAL(databaseOpEnded()), this, SLOT(databaseOpEnded()));

	databaseClosed();
}

Tangerine::~Tangerine() {
	closeDatabase();
}

void Tangerine::setupWindow() {
	setWindowTitle(QString("Tangerine %1 %2.%3").arg(DEV_PHASE).arg(MAJ_VERSION).arg(MIN_VERSION));

	/* central widget */

	mScrollArea = new QScrollArea(NULL);
	mScrollArea->setFrameShape(QFrame::NoFrame);
	mScrollArea->setObjectName("MainScrollArea");
	mScrollArea->setFocusPolicy(Qt::NoFocus);
	mScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
	mScrollArea->setWidgetResizable(true);

	mFrame = new QFrame(NULL);
	mFrame->setFrameShape(QFrame::NoFrame);
	mFrame->setMinimumSize(800, 600);
	mFrame->setObjectName("MainFrame");
	mFrame->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
	mFrame->setStyleSheet("QFrame#MainFrame { background-color: black; }");

	mScrollArea->setWidget(mFrame);
	setCentralWidget(mScrollArea);

	/* create actions */

	createActions();

	/* menu bar */

	mFileMenu = menuBar()->addMenu(tr("&File"));
	mFileMenu->addAction(mLoadDbAct);
	mFileMenu->addAction(mSaveDbAct);
	mFileMenu->addSeparator();
	mFileMenu->addAction(mImportXMLAct);
	mFileMenu->addAction(mSaveXMLAct);

	mHelpMenu = menuBar()->addMenu(tr("&Help"));
	mHelpMenu->addAction(mHelpAboutAct);

	/* toolbar */

	mFileToolbar = addToolBar(tr("File"));
	mFileToolbar->addAction(mLoadDbAct);
	mFileToolbar->addAction(mSaveDbAct);
	mFileToolbar->addSeparator();
	mFileToolbar->addAction(mImportXMLAct);
	mFileToolbar->addAction(mSaveXMLAct);

	mFileToolbar->setMovable(false);

	/* status bar */

	setStatusBar(new QStatusBar());

	/* window size */

	resize(MIN_WIDTH, MIN_HEIGHT);

	//setGeometry(screenSize);
	//QRect screenSize = (QApplication::desktop())->availableGeometry(this);
	//setGeometry(screenSize.x(), screenSize.y(), screenSize.width(), screenSize.height() - (height() - frameGeometry().height()));

	//showMaximized();

	//qDebug() << "frameGeometry:" << frameGeometry() << " | frameSize:" << frameSize();
}

void Tangerine::createActions() {
	mLoadDbAct = new QAction(QIcon(":/rcc/fatcow/32x32/folder_database.png"), tr("&Load database"), this);
	mLoadDbAct->setShortcuts(QKeySequence::Open);
	mLoadDbAct->setStatusTip(tr("Select and load a database"));
    connect(mLoadDbAct, SIGNAL(triggered()), this, SLOT(loadDatabase()));

    mSaveDbAct = new QAction(QIcon(":/rcc/fatcow/32x32/database_save.png"), tr("&Save database"), this);
    mSaveDbAct->setShortcuts(QKeySequence::Save);
    mSaveDbAct->setStatusTip(tr("Save the database in a (new) database file"));
	connect(mSaveDbAct, SIGNAL(triggered()), this, SLOT(saveDatabase()));

	mImportXMLAct = new QAction(QIcon(":/rcc/fatcow/32x32/page_go.png"), tr("&Import from XML"), this);
	mImportXMLAct->setStatusTip(tr("Select and import an XML file"));
	connect(mImportXMLAct, SIGNAL(triggered()), this, SLOT(importDatabase()));

	mSaveXMLAct = new QAction(QIcon(":/rcc/fatcow/32x32/page_save.png"), tr("&Export to XML"), this);
	mSaveXMLAct->setShortcuts(QKeySequence::SaveAs);
	mSaveXMLAct->setStatusTip(tr("Export the current database to an XML file"));
	connect(mSaveXMLAct, SIGNAL(triggered()), this, SLOT(exportDatabase()));

    mHelpAboutAct = new QAction(tr("&About"), this);
    mHelpAboutAct->setStatusTip(tr("Show the about dialog"));
	connect(mHelpAboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

void Tangerine::closeDatabase() {
	if (mDb != NULL) {
		delete mDb;

		mDb = NULL;
	}
}

void Tangerine::loadDatabase() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Open database file or make one"), QString(), QString(), 0, QFileDialog::DontConfirmOverwrite);

	if (fileName != "") {
		mDb->loadFile(fileName);

		if (!mDb->isOpen()) {
			QMessageBox::information(this, tr("Couldn't open database"), tr("Was unable to open database"));

			delete mDb;
		}
	}
}

void Tangerine::saveDatabase() {
	qDebug() << "dummy functionality";

	// what are we supposed to do here? the database is automatically saved no? maybe copy file to other location?
}

void Tangerine::importDatabase() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Choose an XML file to import"), QString(), tr("XML files (*.xml)"), 0, QFileDialog::DontConfirmOverwrite);

	if (fileName != "") {
		mDb->loadFromXML(fileName);
	}
}

void Tangerine::exportDatabase() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("To which file do you want to export?"), QString(), tr("XML files (*.xml)"), 0, QFileDialog::DontConfirmOverwrite);

	if (fileName != "") {
		mDb->saveToXML(fileName);
	}
}

void Tangerine::databaseOpened() {
	mSaveDbAct->setEnabled(true);
	mSaveXMLAct->setEnabled(true);
}

void Tangerine::databaseClosed() {
	mSaveDbAct->setEnabled(false);
	mSaveXMLAct->setEnabled(false);
}

void databaseOpStarted(int steps);
		void databaseOpStepDone();
		void databaseOpEnded();

void Tangerine::databaseOpStarted(const QString& operation, int steps) {
	if (mProgress != NULL) {
		delete mProgress;

		mProgress = NULL;
	}

	mProgress = new QProgressDialog(operation, QString(), 0, steps, this);
	mProgress->setMinimumWidth(400);
	mProgress->setMinimumDuration(0);
	mProgress->setWindowModality(Qt::WindowModal);
	mProgress->show();
}

void Tangerine::databaseOpStepDone(int step) {
	QApplication::processEvents();

	if (mProgress != NULL) {
		mProgress->setValue(step);
	}
}

void Tangerine::databaseOpEnded() {
	if (mProgress != NULL) {
		mProgress->setValue(mProgress->maximum());

		delete mProgress;

		mProgress = NULL;
	}
}

void Tangerine::about() {
	QMessageBox::about(
		this,
		tr("About Tangerine"),
		tr("<b>Tangerine</b> is a next-generation proof of concept GUI for the <b>Thera project</b>. It intends to aid the user in finding and confirming fragment matches.")
	);
}
