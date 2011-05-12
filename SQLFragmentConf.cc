#include "SQLFragmentConf.h"

#include "SQLDatabase.h"

namespace thera {
	SQLFragmentConf::SQLFragmentConf(SQLDatabase *db, int id) : mDb(db), mId(id) { }
	SQLFragmentConf::~SQLFragmentConf() { }
	SQLFragmentConf::SQLFragmentConf(const SQLFragmentConf& that) : IFragmentConf(that), mDb(that.mDb), mId(that.mId) { }
	SQLFragmentConf& SQLFragmentConf::operator=(const SQLFragmentConf& that) {
		if (this != &that) {
			IFragmentConf::operator=(that);

			mDb = that.mDb;
			mId = that.mId;
		}

		return *this;
	}

	void SQLFragmentConf::updateID(int id) {
		mId = id;
	}

	bool SQLFragmentConf::setMetaData(const QString& field, const QString& value) const {
		assert(mId != -1 && mDb != NULL);

		if (mDb->matchHasField(field)) {
			mDb->matchSetValue(mId, field, value);
		}
		else {
			return false;
		}

		return true;
	}

	bool SQLFragmentConf::setMetaData(const QString& field, double value) const {
		assert(mId != -1 && mDb != NULL);

		if (mDb->matchHasField(field)) {
			mDb->matchSetValue(mId, field, value);
		}
		else {
			return false;
		}

		return true;
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

    	if (mDb->matchHasField(field)) {
    		return mDb->matchGetValue<T>(mId, field);
    	}

    	return deflt;
    }
}
