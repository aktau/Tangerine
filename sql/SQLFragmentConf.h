#ifndef SQLFRAGMENTCONF_H_
#define SQLFRAGMENTCONF_H_

#include "IFragmentConf.h"

#include <assert.h>

#include <QMap>
#include <QVariant>

class SQLDatabase;

namespace thera {
	class SQLFragmentConf: public IFragmentConf {
		public:
			SQLFragmentConf(SQLDatabase *db = NULL, int id = -1);
			SQLFragmentConf(SQLDatabase *db, int id, int *fragments, float relevance, const XF& xf, const vec3& CP = illegal<vec3>(), float CPRadius = illegal<float>());
			SQLFragmentConf(SQLDatabase *db, const QMap<QString, QVariant>& cache, int id, int *fragments, float relevance, const XF& xf, const vec3& CP = illegal<vec3>(), float CPRadius = illegal<float>());

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

			bool isValid() const;

			// absorb the stored values of another fragment conf
			// will return false if a value did not match with a value
			// already stored in the object. Nevertheless, this fragment conf
			// WILL absorb all values regardless of conflicts, unless the indices of the objects
			// are different, in which case it will absorb nothing
			// this method is mostly for usage by the models in their refresh signal emitters
			virtual bool absorb(const SQLFragmentConf& other);

		private:
			template<typename T> T get(const QString &field, T deflt) const;
			template<typename T> bool set(const QString &field, T value) const;

		private:
			SQLDatabase *mDb;

			// the cache is supposed to be transparant, so we can declare it mutable for some const-correctness
			typedef QMap<QString, QVariant> CacheMap;
			mutable CacheMap mCache;

			int mId;
	};
}

#endif /* SQLFRAGMENTCONF_H_ */
