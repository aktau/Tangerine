#include "Tangerine.h"

#include "SQLDatabase.h"

#include <QtGui>
#include <QApplication>

#include <QDebug>

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(theragui);
    Q_INIT_RESOURCE(tangerine);

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
