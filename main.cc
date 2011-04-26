#include <stdlib.h>

#include <QtGui>
#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QDir>

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

	const char *env = getenv("THERA_DB");

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

void parseCommandLine(int argc, char *argv[], QDir& thumbDir) {
    for (int i = 1; i < argc; ++i) {
    	const QString argument = argv[i];

    	if (argument == "--thumbDir" || argument == "-t") {
    		if (i + 1 < argc) {
    			// set thumbdir to cmdline argument AND advance pointers
    			thumbDir = QDir(argv[++i]);
    		}
    	}
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
    QDir thumbDir("C:\\Documents and Settings\\Administrator\\My Documents\\dump-sw50_3_16-20100606");

    initializeFragmentDb();
    parseCommandLine(argc, argv, thumbDir);

    SQLiteDatabase db;

    Tangerine window(db, thumbDir);
    window.show();

    return application.exec();
}
