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
			virtual int index() const; // index is something different than ID in the original FragmentConf, but in SQLFragmentConf we define it to be the same
			virtual void updateID(int id);
			virtual int getID() const;

			// TODO: these methods are maybe technically const, but they are not semantically const, however currently FragmentConf dictates otherwise
			virtual bool setMetaData(const QString &field, const QString &value) const;
			virtual bool setMetaData(const QString &field, double value) const;

			virtual QString getString(const QString &field, const QString &deflt = "") const;
			virtual double getDouble(const QString &field, double deflt = 0.0) const;
			virtual int getInt(const QString &field, int deflt = 0) const;

		private:
			template<typename T> T get(const QString &field, T deflt) const;

		private:
			SQLDatabase *mDb;

			int mId;
	};
}

#endif /* SQLFRAGMENTCONF_H_ */
