#include <stdlib.h>

#include <QtGui>
#include <QApplication>
#include <QDebug>
#include <QSettings>

#include "Tangerine.h"
#include "SQLiteDatabase.h"

#include "Database.h"

using namespace thera;

void initializeFragmentDb() {
	QSettings settings;
	QString dbDir;

	if (settings.contains(SETTINGS_DB_ROOT_KEY)) {
		dbDir = settings.value(SETTINGS_DB_ROOT_KEY, QString()).toString();

		if (Database::init(dbDir, Database::FRAGMENT, true)) {
			settings.setValue(SETTINGS_DB_ROOT_KEY, dbDir);
			return;
		}
	}

	char *env = getenv("THERA_DB");
	if (env) {
		dbDir = env;

		if (Database::init(dbDir, Database::FRAGMENT, true)) {
			settings.setValue(SETTINGS_DB_ROOT_KEY, dbDir);
			return;
		}
	}

	if (!dbDir.isEmpty()) {
		settings.setValue(SETTINGS_DB_ROOT_KEY, dbDir);
	}
}

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

    QSettings settings;

    QApplication application(argc, argv);

    initializeFragmentDb();

    SQLiteDatabase db;

    Tangerine window(db);
    window.show();

    return application.exec();
}
