/*
 * Tangerine.cpp
 *
 *  Created on: 11-apr.-2011
 *      Author: Aktau
 */

#include "Tangerine.h"

#include <QtGui>
#include <QDebug>

Tangerine::Tangerine(QWidget *parent) : QMainWindow(parent) {
	QString fileName = QFileDialog::getSaveFileName(NULL, tr("Open Database"));
	qDebug() << "Selected: " << fileName;
}

Tangerine::~Tangerine() {
	// TODO Auto-generated destructor stub
}
