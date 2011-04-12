#include "Tangerine.h"

#include "SQLDatabase.h"

#include <QtGui>
#include <QApplication>

#include <QDebug>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(theragui);

#ifdef Q_WS_MAX
    QCoreApplication::setOrganizationName("princeton.edu"); // Mac OS X requires URL instead of name
#else
    QCoreApplication::setOrganizationName("Princeton");
#endif
    QCoreApplication::setOrganizationDomain("princeton.edu");
    QCoreApplication::setApplicationName("browsematches");

    QApplication application(argc, argv);

    Tangerine window;
    window.show();

    return application.exec();
}
