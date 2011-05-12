#ifndef INVALIDFRAGMENTCONF_H_
#define INVALIDFRAGMENTCONF_H_

#include "IFragmentConf.h"

namespace thera {
	class InvalidFragmentConf : public IFragmentConf  {
		public:
			InvalidFragmentConf() { }
			virtual ~InvalidFragmentConf() { }

		public:
			virtual void updateID(int id) { qDebug() << "InvalidFragmentConf::updateID: tried to set id" << id; }

			virtual bool setMetaData(const QString &field, const QString &value) const { qDebug() << "InvalidFragmentConf::setMetaData: " << field << "|" << value; return false; }
			virtual bool setMetaData(const QString &field, double value) const { qDebug() << "InvalidFragmentConf::setMetaData: " << field << "|" << value; return false; }
			virtual QString getString(const QString &field, const QString &deflt="") const { qDebug() << "InvalidFragmentConf::getString: " << field << "|" << deflt; return deflt; }
			virtual double getDouble(const QString &field, double deflt=0.0) const { qDebug() << "InvalidFragmentConf::getDouble: " << field << "|" << deflt; return deflt; }
			virtual int getInt(const QString &field, int deflt = 0) const { qDebug() << "InvalidFragmentConf::getInt: " << field << "|" << deflt; return deflt; };

			virtual const QString getTargetId() const { return QString(); }
			virtual const QString getSourceId() const { return QString(); }
	};
}

#endif /* INVALIDFRAGMENTCONF_H_ */
