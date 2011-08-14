#include "SQLDatabaseBenchmarker.h"

#include <QDebug>

SQLDatabaseBenchmarker::SQLDatabaseBenchmarker(SQLDatabase *db) {
	setDatabase(db);
}

SQLDatabaseBenchmarker::~SQLDatabaseBenchmarker() { }

void SQLDatabaseBenchmarker::setDatabase(SQLDatabase *db) {
	mDb = db;
}

void SQLDatabaseBenchmarker::setPreloadConfigurations(const QList<QStringList>& preloadConfigurations) {
	mPreloadConfigurations = preloadConfigurations;

	if (preloadConfigurations.isEmpty()) {
		mPreloadConfigurations << QStringList();
	}
}

void SQLDatabaseBenchmarker::setParameterConfigurations(const QList<ModelParameters>& parameterConfigurations) {
	mParameterConfigurations = parameterConfigurations;

	if (parameterConfigurations.isEmpty()) {
		mParameterConfigurations << ModelParameters();
	}
}

void SQLDatabaseBenchmarker::setRepetitionConfigurations(const QList< QList< QPair<int, int> > >& repetitionConfigurations) {
	mRepetitionConfigurations = repetitionConfigurations;

	if (repetitionConfigurations.isEmpty()) {
		mRepetitionConfigurations << (WindowList() << WindowPair(0,20) << WindowPair(20,20) << WindowPair(1000, 20) << WindowPair(2000,20) << WindowPair(10000,20) << WindowPair(500,20));
	}
}

void SQLDatabaseBenchmarker::start(const QString& filename) {
	if (!mDb) return;

	{
		QFile file(filename + ".warmup");
		file.open(QIODevice::WriteOnly | QIODevice::Text);
		QTextStream stream(&file);

		qDebug() << "[NEXT RUN] Dry run";
		mDb->setOptions(SQLDatabase::UseLateRowLookup | SQLDatabase::UseViewEncapsulation | SQLDatabase::ForcePrimaryIndex);
		run(stream);
	}

	{
		QFile file(filename + ".real");
		file.open(QIODevice::WriteOnly | QIODevice::Text);
		QTextStream stream(&file);

		qDebug() << "[NEXT RUN] Using all optimizations";
		run(stream);
	}

	mDb->materializeMetaAttributes();

	{
		QFile file(filename + ".materialized");
		file.open(QIODevice::WriteOnly | QIODevice::Text);
		QTextStream stream(&file);

		qDebug() << "[NEXT RUN] Materialized";
		run(stream);
	}


	/*
	{
		QFile file(filename + ".views");
		file.open(QIODevice::WriteOnly | QIODevice::Text);
		QTextStream stream(&file);

		qDebug() << "[NEXT RUN] Using views";
		mDb->setOptions(SQLDatabase::UseViewEncapsulation);
		run(stream);
	}

	{
		QFile file(filename + ".noopt");
		file.open(QIODevice::WriteOnly | QIODevice::Text);
		QTextStream stream(&file);

		qDebug() << "[NEXT RUN] Using no optimizations";
		mDb->setOptions(0);
		run(stream);
	}
	*/
}

template<typename T>
void SQLDatabaseBenchmarker::run(T& stream) {
	QElapsedTimer timer;

	foreach (const WindowList& windows, mRepetitionConfigurations) {
		// write window header
		int i = 0;

		stream << "\nWindows, ";
		foreach (const WindowPair& window, windows) {
			stream << "[" << window.first << "," << window.second << "], ";
		}
		stream << "\n";

		foreach (const QStringList& preload, mPreloadConfigurations) {
			mPreloadFields = preload;

			foreach (const ModelParameters& parameters, mParameterConfigurations) {
				mPar = parameters;

				stream << "[" << ++i << "] " << " [PRELOADING: " << mPreloadFields.join(", ") << "]" << " with parameters [" << mPar.toString() << "]" << " [paginated / non-paginated (alternate)]";
				//if (paginate) stream << " paginated";
				stream << "\n";

				qDebug() << "[" << ++i << "] " << " [PRELOADING: " << mPreloadFields.join(", ") << "]" << " with parameters [" << mPar.toString() << "]" << " [paginated / non-paginated (alternate)]";

				for (int paginate = 3; paginate >= 0; --paginate) {
					mMatches.clear();

					qDebug() << "---> PASS:" << paginate << paginate % 2;

					timer.start();
					foreach (const WindowPair& window, windows) {
						if (doPass(window.first, window.second, paginate % 2)) {
							stream << timer.restart() << ", ";
						}
						else {
							stream << "invalid, ";
						}


						stream.flush();
					}

					stream << "\n";
				}
			}
		}
	}
}

bool SQLDatabaseBenchmarker::doPass(int requestedWindowBegin, int requestedWindowSize, bool paginate) {
	SQLQueryParameters parameters = SQLQueryParameters(mPreloadFields, mPar.sortField, mPar.sortOrder, mPar.filter);

	int loadedWindowEnd = mLoadedWindowBegin + mMatches.size();
	int requestedWindowEnd = requestedWindowBegin + requestedWindowSize;

	if (paginate && !mMatches.isEmpty() && requestedWindowSize > 0) {
		qDebug("SQLDatabaseBenchmarker::start: [PAGINATION] moving window [%d,%d] to window [%d,%d]", mLoadedWindowBegin, loadedWindowEnd, requestedWindowBegin, requestedWindowEnd);
		// find the reference fragment

		// detect special case where the old window is inside of the new window (and the new window is bigger on both sides)
		if (requestedWindowBegin < mLoadedWindowBegin && requestedWindowEnd > loadedWindowEnd) {
			qDebug("SQLDatabaseBenchmarker::start: [PAGINATION -> STANDARD] fail, older window strictly smaller [%d,%d] than new window [%d,%d], switching to standard query", mLoadedWindowBegin, loadedWindowEnd, requestedWindowBegin, requestedWindowEnd);

			parameters.moveToAbsoluteWindow(requestedWindowBegin, requestedWindowSize);

			qDebug("SQLDatabaseBenchmarker::start: [STANDARD] done");
		}
		else {
			bool forward;
			bool inclusive;
			int referenceIndex;
			int offset;

			// in all the other cases, we can request the new window in one go
			if (requestedWindowBegin >= mLoadedWindowBegin) {
				forward = true;

				// can be fetched completely by forward searching (looking ahead)
				inclusive = requestedWindowBegin == mLoadedWindowBegin;

				referenceIndex = qMin(loadedWindowEnd, requestedWindowBegin) - 1 + inclusive;
				offset = qMax(0, requestedWindowBegin - loadedWindowEnd);
			}
			else if (requestedWindowEnd <= loadedWindowEnd) {
				forward = false;

				// can be fetched completely by backward searching
				inclusive = requestedWindowEnd == loadedWindowEnd;

				referenceIndex = qMax(mLoadedWindowBegin, requestedWindowEnd) - inclusive;
				offset = qMax(0, mLoadedWindowBegin - requestedWindowEnd);
			}
			else {
				qDebug("SQLDatabaseBenchmarker::start: [PAGINATION] shouldn't be here: [%d,%d] to window [%d,%d]", mLoadedWindowBegin, loadedWindowEnd, requestedWindowBegin, requestedWindowEnd);
			}

			const thera::SQLFragmentConf &conf = mMatches.at((referenceIndex - 0) % mMatches.size());

			//int matchIdExtremeValue = conf.index();
			//double sortFieldExtremeValue = conf.getDouble(mPar.sortField, 0.0);

			parameters.moveToRelativeWindow(conf, inclusive, forward, offset, requestedWindowSize);
		}
	}
	else {
		qDebug("SQLDatabaseBenchmarker::start: [STANDARD] moving window [%d,%d] to window [%d,%d]", mLoadedWindowBegin, loadedWindowEnd, requestedWindowBegin, requestedWindowEnd);
		parameters.moveToAbsoluteWindow(requestedWindowBegin, requestedWindowSize);
	}

	mMatches = mDb->getMatches(parameters);

	// save which window is currently active
	mLoadedWindowBegin = requestedWindowBegin;

	return !mMatches.isEmpty();
}
