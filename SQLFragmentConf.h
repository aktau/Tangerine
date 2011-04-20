#ifndef SQLFRAGMENTCONF_H_
#define SQLFRAGMENTCONF_H_

#include "IFragmentConf.h"

#include <assert.h>

class SQLDatabase;

namespace thera {
	class SQLFragmentConf: public IFragmentConf {
		public:
			SQLFragmentConf(SQLDatabase *db, int id);
			virtual ~SQLFragmentConf();
			SQLFragmentConf(const SQLFragmentConf&);
			virtual SQLFragmentConf& operator=(const SQLFragmentConf&);

		//private:
		//	 // leave it to the constructor, for now...

		public:
			virtual void updateID(int id);
			virtual bool setMetaData(const QString &field, const QString &value) const;
			virtual bool setMetaData(const QString &field, double value) const;
			virtual QString getString(const QString &field, const QString &deflt = "") const;
			virtual double getDouble(const QString &field, double deflt = 0.0) const;

		private:
			SQLDatabase *mDb;

			int mId;
	};
}

#endif /* SQLFRAGMENTCONF_H_ */
