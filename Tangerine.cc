#include "Tangerine.h"
#include "SQLDatabase.h"

#include <QtGui>
#include <QDebug>

Tangerine::Tangerine(QWidget *parent) : QMainWindow(parent) {
	QString fileName = QFileDialog::getSaveFileName(NULL, tr("Open Database"));
	qDebug() << "Selected: " << fileName;

	SQLDatabase db(fileName);

	//SQLDatabase *db = new SQLDatabase(fileName);
}

Tangerine::~Tangerine() {
	// TODO Auto-generated destructor stub
}
