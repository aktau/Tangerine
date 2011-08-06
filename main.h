#ifndef MAIN_H_
#define MAIN_H_

#define SETTINGS_DB_ROOT_KEY "db/root"
#define SETTINGS_DB_IMAGECACHE_KEY "matchdb/images"
#define SETTINGS_DB_LASTMATCHDB_KEY "matchdb/root"
#define SETTINGS_APP_AUTOLOAD_MATCHDB_KEY "app/autoload/matchdb"

#define DEV_PHASE "Alpha"
#define MAJ_VERSION 0
#define MIN_VERSION 5

#include <QtGui/QApplication>
#include <QEvent>
#include <QtDebug>

class TangerineApplication : public QApplication {
	Q_OBJECT

	public:
		TangerineApplication(int argc, char * argv[]) : QApplication( argc, argv ) {}

		bool event(QEvent * pEvent) {
			if (pEvent->type() == QEvent::ApplicationActivate) {
				qDebug() << "ApplicationActivate";
			}
			else if (pEvent->type() == QEvent::ApplicationDeactivate) {
				qDebug() << "ApplicationDeactivate";
			}

			return QApplication::event(pEvent);
		}
};

#endif /* MAIN_H_ */
