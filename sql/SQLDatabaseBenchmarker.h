#ifndef SQLDATABASEBENCHMARKER_H_
#define SQLDATABASEBENCHMARKER_H_

#include <QList>
#include <QTextStream>
#include <QDataStream>

#include "SQLDatabase.h"

#include "ModelParameters.h"

class SQLDatabaseBenchmarker {
	public:
		SQLDatabaseBenchmarker(SQLDatabase *db);
		virtual ~SQLDatabaseBenchmarker();

	public:
		virtual void setDatabase(SQLDatabase *db);
		virtual void setPreloadConfigurations(const QList<QStringList>& preloadConfigurations);
		virtual void setParameterConfigurations(const QList<ModelParameters>& parameterConfigurations);
		virtual void setRepetitionConfigurations(const QList< QList< QPair<int, int> > >& repetitionConfigurations);
		virtual void start(const QString& file); // will perform a benchmark of common queries and write them out to file

	protected:
		//virtual void run(QTextStream& stream);
		template<typename T> void run(T& stream);
		virtual bool doPass(int requestedWindowBegin, int requestedWindowSize, bool paginate);

	protected:
		typedef QPair<int, int> WindowPair;
		typedef QList<WindowPair> WindowList;

		int mLoadedWindowBegin;

		QList<thera::SQLFragmentConf> mMatches;

		SQLDatabase *mDb;
		QList<QStringList> mPreloadConfigurations;
		QList<ModelParameters> mParameterConfigurations;
		QList< QList< QPair<int, int> > > mRepetitionConfigurations;

		QStringList mPreloadFields;
		ModelParameters mPar;
};

#endif /* SQLDATABASEBENCHMARKER_H_ */
