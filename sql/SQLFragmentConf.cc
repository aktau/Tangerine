#include "SQLFragmentConf.h"

#include "SQLDatabase.h"

namespace thera {
	SQLFragmentConf::SQLFragmentConf(SQLDatabase *db, int id) : mDb(db), mId(id) { }
	SQLFragmentConf::SQLFragmentConf(SQLDatabase *db, int id, int *fragments, float relevance, const XF& xf, const vec3& CP, float CPRadius)
		: IFragmentConf(fragments, relevance, xf, CP, CPRadius), mDb(db), mId(id) {}
	SQLFragmentConf::SQLFragmentConf(SQLDatabase *db, const QMap<QString, QVariant>& cache, int id, int *fragments, float relevance, const XF& xf, const vec3& CP, float CPRadius)
		: IFragmentConf(fragments, relevance, xf, CP, CPRadius), mDb(db), mCache(cache), mId(id) {}
	SQLFragmentConf::~SQLFragmentConf() { }
	SQLFragmentConf::SQLFragmentConf(const SQLFragmentConf& that) : IFragmentConf(that), mDb(that.mDb), mCache(that.mCache), mId(that.mId) { }
	SQLFragmentConf& SQLFragmentConf::operator=(const SQLFragmentConf& that) {
		if (this != &that) {
			IFragmentConf::operator=(that);

			mDb = that.mDb;
			mId = that.mId;
			mCache = that.mCache;
		}

		return *this;
	}

	void SQLFragmentConf::updateID(int id) {
		mId = id;
	}

	int SQLFragmentConf::index() const {
		return getID();
	}

	int SQLFragmentConf::getID() const {
		return mId;
	}

	bool SQLFragmentConf::setMetaData(const QString& field, const QString& value) const {
		return set(field, value);
	}

	bool SQLFragmentConf::setMetaData(const QString& field, double value) const {
		return set(field, value);
	}

	QString SQLFragmentConf::getString(const QString& field, const QString& deflt) const {
		return get<QString>(field, deflt);
	}

    double SQLFragmentConf::getDouble(const QString& field, double deflt) const {
    	return get<double>(field, deflt);
    }

    int SQLFragmentConf::getInt(const QString &field, int deflt) const {
    	return get<int>(field, deflt);
    }

    template<typename T> inline T SQLFragmentConf::get(const QString &field, T deflt) const {
    	assert(mId != -1 && mDb != NULL);

    	if (!mDb->matchHasField(field)) {
    		qDebug() << "SQLFragmentConf::get: match doesn't have field" << field;

    		return deflt;
    	}

    	CacheMap::const_iterator i = mCache.constFind(field);

    	if (i == mCache.constEnd()) {
			i = mCache.insert(field, mDb->matchGetValue<T>(mId, field, deflt));

			//qDebug() << "SQLFragmentConf::get: uncached hit for" << field << "|" << mId << "|" << i.value().toString();
    	}
    	else {
    		//qDebug() << "SQLFragmentConf::get: cached hit for" << field << "|" << mId << "|" << i.value().toString();
    	}

    	return i.value().value<T>();
    }

    template<typename T> inline bool SQLFragmentConf::set(const QString &field, T value) const {
    	assert(mId != -1 && mDb != NULL);

		if (!mDb->matchHasField(field)) {
			qDebug() << "SQLFragmentConf::setMetaData: field" << field << "was not found in" << mDb->matchFields() << ", tried to insert" << value;

			return false;
		}
		else {
			// cache is write-through because there are too many things we can't control
			mCache.insert(field, value);

			mDb->matchSetValue(mId, field, value);
		}

		return true;
    }

    bool SQLFragmentConf::isValid() const {
    	return (mId != -1) && (mDb != NULL);
    }

    bool SQLFragmentConf::absorb(const SQLFragmentConf& other) const {
    	if (mId != other.mId) {
    		qDebug() << "SQLFragmentConf::absorb: Id's did not match:" << mId << "vs other" << other.mId;

    		return false;
    	}

    	bool changed = false;

    	// compare caches and absorb values
    	CacheMap::iterator thisPair = mCache.end();

    	for (CacheMap::const_iterator otherPair = other.mCache.constBegin(); otherPair != other.mCache.constEnd(); ++otherPair) {
    		thisPair = mCache.find(otherPair.key());

    		if (thisPair != mCache.end()) {
    			// both keys exist, compare values
    			//qDebug() << "SQLFragmentConf::absorb: Comparing" << mId << ":" << thisPair.key() << "->" << thisPair.value() << "and" << otherPair.key() << "->" << otherPair.value();

    			if (thisPair.value() != otherPair.value()) {
    				//qDebug() << "SQLFragmentConf::absorb: CHANGE DETECTED!!!!!!!!!!!!!!!!!!!\n\n\n\n\n\nCHAAAAAAAAAAAAAANGE";

    				thisPair.value() = otherPair.value();

    				changed = true;
    			}
    		}
    		else {
    			// key not found, insert
    			// note that this DOESN'T imply changed == true, because we don't know what the value would have been
    			// it was just not in the cache

    			mCache.insert(otherPair.key(), otherPair.value());
    		}
    	}

    	//qDebug() << "SQLFragmentConf::absorb: return changed =" << changed;

    	// if changed == true, then a conflicting value was merged in, and we return false
    	// if changed == false, then we have just a possibly larger cache with no different values, return true
    	return !changed;
    }

    void SQLFragmentConf::clearCache(const QString& field) const {
    	if (field.isEmpty()) {
    		mCache.clear();
    	}
    	else {
    		mCache.remove(field);
    	}
    }
}
