#include "Tangerine.h"

#include <QtGui>
#include <QDebug>

#include <assert.h>

#include "SQLFragmentConf.h"
#include "Database.h"

using namespace thera;

const QString Tangerine::MATCH_COUNT_TEXT = "%1 matches loaded";

const int Tangerine::MIN_WIDTH = 1024;
const int Tangerine::MIN_HEIGHT = 786;

Tangerine::Tangerine(SQLDatabase& db, QWidget *parent) : QMainWindow(parent), mDb(db), mProgress(NULL), mNumberOfMatchesLabel(NULL) {
	setupWindow();

	// the ordering is important, the slots use instances made in setupWindow() et cetera

	connect(&mDb, SIGNAL(databaseOpened()), this, SLOT(databaseOpened()));
	connect(&mDb, SIGNAL(databaseClosed()), this, SLOT(databaseClosed()));
	connect(&mDb, SIGNAL(databaseOpStarted(const QString&, int)), this, SLOT(databaseOpStarted(const QString&, int)));
	connect(&mDb, SIGNAL(databaseOpStepDone(int)), this, SLOT(databaseOpStepDone(int)));
	connect(&mDb, SIGNAL(databaseOpEnded()), this, SLOT(databaseOpEnded()));
	//connect(&mDb, SIGNAL(matchCountChanged()), this, SLOT(updateStatusBar()));
	connect(&mDb, SIGNAL(matchCountChanged()), this, SLOT(matchCountChanged()));

	databaseClosed();

	mLoadFragDbAct->setEnabled(Database::isValid() ? false : true);
	mLoadMatchDbAct->setEnabled(Database::isValid() ? true : false);
	mImportXMLAct->setEnabled(Database::isValid() ? true : false);
}

Tangerine::~Tangerine() {
	closeDatabase();
}

void Tangerine::setupWindow() {
	/* window configuration */

	setWindowIcon(QIcon("tangerine.ico"));
	setWindowTitle(QString("Tangerine %1 %2.%3").arg(DEV_PHASE).arg(MAJ_VERSION).arg(MIN_VERSION));
	setToolButtonStyle(Qt::ToolButtonIconOnly);

	/* central widget */

	//mTileView = new MatchTileView(QDir("E:\\Thesis\\tongeren_vrijthof_db\\cache\\ribbonmatcher\\dump-sw50_3_16-20100606"));
	mTileView = new MatchTileView(QDir("C:\\Documents and Settings\\Administrator\\My Documents\\dump-sw50_3_16-20100606"));
	mTileView->setModel(&mModel);

	mCentralWidget = new QStackedWidget;
	mCentralWidget->addWidget(mTileView);
#ifdef WITH_GRAPH
	mGraphView = new GraphView;
	mGraphView->setModel(&mModel);

	mCentralWidget->addWidget(mGraphView);
#else
	mCentralWidget->addWidget(new QWidget);
#endif


	setCentralWidget(mCentralWidget);

	normalView();

	/* create actions */

	createActions();

	/* menu bar */

	mFileMenu = menuBar()->addMenu(tr("&File"));
	mFileMenu->addAction(mLoadFragDbAct);
	mFileMenu->addAction(mLoadMatchDbAct);
	mFileMenu->addAction(mSaveDbAct);
	mFileMenu->addSeparator();
	mFileMenu->addAction(mImportXMLAct);
	mFileMenu->addAction(mSaveXMLAct);

	mViewMenu = menuBar()->addMenu(tr("&View"));
	mViewMenu->addAction(mNormalViewAct);
	mViewMenu->addAction(mNodeViewAct);

	mHelpMenu = menuBar()->addMenu(tr("&Help"));
	mHelpMenu->addAction(mHelpAboutAct);

	/* toolbar */

	mFileToolbar = addToolBar(tr("File"));
	mFileToolbar->addAction(mLoadFragDbAct);
	mFileToolbar->addAction(mLoadMatchDbAct);
	mFileToolbar->addAction(mSaveDbAct);
	mFileToolbar->addSeparator();
	mFileToolbar->addAction(mImportXMLAct);
	mFileToolbar->addAction(mSaveXMLAct);
	mFileToolbar->addSeparator();
	mFileToolbar->addAction(mNormalViewAct);
	mFileToolbar->addAction(mNodeViewAct);

	mFileToolbar->setMovable(false);

	/* status bar */

	setStatusBar(new QStatusBar());

	mNumberOfMatchesLabel = new QLabel();
	//mNumberOfMatchesLabel->setText(MATCH_COUNT_TEXT.arg(0));

	statusBar()->addPermanentWidget(mNumberOfMatchesLabel);

	updateStatusBar();

	/* styles */
	QFile file(":/rcc/stylesheet.qss");
	file.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(file.readAll());
	file.close();

	setStyleSheet(styleSheet);

	/* window size */

	resize(MIN_WIDTH, MIN_HEIGHT);

	//setGeometry(screenSize);
	//QRect screenSize = (QApplication::desktop())->availableGeometry(this);
	//setGeometry(screenSize.x(), screenSize.y(), screenSize.width(), screenSize.height() - (height() - frameGeometry().height()));

	//showMaximized();

	//qDebug() << "frameGeometry:" << frameGeometry() << " | frameSize:" << frameSize();
}

void Tangerine::createActions() {
	mLoadFragDbAct = new QAction(QIcon(":/rcc/fatcow/32x32/folder_table.png"), tr("Load &fragment database"), this);
	mLoadFragDbAct->setStatusTip(tr("Select and load a fragment database"));
	connect(mLoadFragDbAct, SIGNAL(triggered()), this, SLOT(loadFragmentDatabase()));

	mLoadMatchDbAct = new QAction(QIcon(":/rcc/fatcow/32x32/folder_database.png"), tr("Load &match database"), this);
	mLoadMatchDbAct->setShortcuts(QKeySequence::Open);
	mLoadMatchDbAct->setStatusTip(tr("Select and load a match database"));
    connect(mLoadMatchDbAct, SIGNAL(triggered()), this, SLOT(loadMatchDatabase()));

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

	mNormalViewAct = new QAction(QIcon(":/rcc/fatcow/32x32/things_digital.png"), tr("Switch to &normal view"), this);
	mNormalViewAct->setCheckable(true);
	mNormalViewAct->setStatusTip(tr("Switch to normal view"));
	connect(mNormalViewAct, SIGNAL(triggered()), this, SLOT(normalView()));

	mNodeViewAct = new QAction(QIcon(":/rcc/fatcow/32x32/bubblechart.png"), tr("Switch to n&ode view"), this);
	mNodeViewAct->setCheckable(true);
	mNodeViewAct->setStatusTip(tr("Switch to node view"));
	connect(mNodeViewAct, SIGNAL(triggered()), this, SLOT(nodeView()));

	mViewGroup = new QActionGroup(this);
	mViewGroup->addAction(mNormalViewAct);
	mViewGroup->addAction(mNodeViewAct);
	mNormalViewAct->setChecked(true);

    mHelpAboutAct = new QAction(QIcon(":/rcc/fatcow/32x32/information.png"), tr("&About"), this);
    mHelpAboutAct->setStatusTip(tr("Show the about dialog"));
	connect(mHelpAboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

/**
 * TODO: fill in
 */
void Tangerine::closeDatabase() {
	/*
	if (mDb != NULL) {
		//delete mDb;

		mDb = NULL;
	}
	*/
}

void Tangerine::updateStatusBar() {
	mNumberOfMatchesLabel->setText(MATCH_COUNT_TEXT.arg(mDb.matchCount()));
}

void Tangerine::loadFragmentDatabase() {
	QSettings settings;

	QString dbDir = QFileDialog::getExistingDirectory(
		this,
		QObject::tr("Choose the fragment database root directory"),
		QString(),
		QFileDialog::ShowDirsOnly |
		QFileDialog::DontResolveSymlinks |
		QFileDialog::DontConfirmOverwrite
	);

	if (!dbDir.isEmpty() && Database::init(dbDir, Database::FRAGMENT, true)) {
		settings.setValue(SETTINGS_DB_ROOT_KEY, dbDir);

		emit fragmentDatabaseOpened();
	}
}


void Tangerine::loadMatchDatabase() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Open database file or make one"), QString(), QString(), 0, QFileDialog::DontConfirmOverwrite);

	if (fileName != "") {
		mDb.connect(fileName);

		if (!mDb.isOpen()) {
			QMessageBox::information(this, tr("Couldn't open database"), tr("Was unable to open database"));
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
		mDb.loadFromXML(fileName);
	}
}

void Tangerine::exportDatabase() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("To which file do you want to export?"), QString(), tr("XML files (*.xml)"), 0);

	if (fileName != "") {
		mDb.saveToXML(fileName);
	}
}

void Tangerine::normalView() {
	mCentralWidget->setCurrentIndex(0);
}

void Tangerine::nodeView() {
	mCentralWidget->setCurrentIndex(1);
}

void Tangerine::fragmentDatabaseOpened() {
	mLoadFragDbAct->setEnabled(false);
	mLoadMatchDbAct->setEnabled(true);
	mImportXMLAct->setEnabled(true);
}

void Tangerine::matchCountChanged() {
	mModel.setMatches(mDb.getAllMatches());

	qDebug() << "Got all matches! There are" << mModel.size();

	updateStatusBar();
}

void Tangerine::databaseOpened() {
	mSaveDbAct->setEnabled(true);
	mSaveXMLAct->setEnabled(true);
}

void Tangerine::databaseClosed() {
	mSaveDbAct->setEnabled(false);
	mSaveXMLAct->setEnabled(false);
}

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
