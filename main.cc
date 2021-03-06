#include <stdlib.h> // for getenv()

#include <QtGui>
#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QDir>

#include "main.h"

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
    QCoreApplication::setOrganizationName("kuleuven.be"); // Mac OS X requires URL instead of name
#else
    QCoreApplication::setOrganizationName("kuleuven.be");
#endif
    QCoreApplication::setOrganizationDomain("kuleuven.be");
    QCoreApplication::setApplicationName("Tangerine");

    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();

    QSettings settings;
    TangerineApplication application(argc, argv);
    QDir thumbDir = settings.value(SETTINGS_DB_IMAGECACHE_KEY, "C:\\Documents and Settings\\Administrator\\My Documents\\dump-sw50_3_16-20100606").toString();

    parseCommandLine(argc, argv, thumbDir);

    //SQLDatabase *db = SQLDatabase::getDatabase(QCoreApplication::instance());

    //Tangerine window(db, thumbDir);
    Tangerine window(thumbDir);
    window.show();

    return application.exec();
}
