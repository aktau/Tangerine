#include "Tangerine.h"
#include "SQLDatabase.h"

#include <QtGui>
#include <QDebug>

const int Tangerine::MIN_WIDTH = 1024;
const int Tangerine::MIN_HEIGHT = 786;

Tangerine::Tangerine(QWidget *parent) : QMainWindow(parent) {
	setupWindow();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Open database file or make one"), QString(), QString(), 0, QFileDialog::DontConfirmOverwrite);
	qDebug() << "Selected:" << fileName;

	SQLDatabase db(fileName);

	if (db.isOpen()) {
		db.loadFromXML("db/matches.xml");
	}
}

Tangerine::~Tangerine() {
	/* nothing yet */
}

void Tangerine::setupWindow() {
	setWindowTitle(QString("Tangerine %1 %2.%3").arg(DEV_PHASE).arg(MAJ_VERSION).arg(MIN_VERSION));

	mScrollArea = new QScrollArea(NULL);
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

	resize(MIN_WIDTH, MIN_HEIGHT);

	//setGeometry(screenSize);
	//QRect screenSize = (QApplication::desktop())->availableGeometry(this);
	//setGeometry(screenSize.x(), screenSize.y(), screenSize.width(), screenSize.height() - (height() - frameGeometry().height()));

	//showMaximized();

	//qDebug() << "frameGeometry:" << frameGeometry() << " | frameSize:" << frameSize();
}
