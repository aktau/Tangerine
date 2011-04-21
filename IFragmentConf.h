#ifndef IFRAGMENTCONF_H_
#define IFRAGMENTCONF_H_

#include <QObject>

#include "Database.h"
#include "Fuzzy.h"
#include "XF.h"

namespace thera {
	 // This is an interface class with a few fields here and there (more like a trait)
	 // known inherits: XmlFragmentConf (to be made), SQLFragmentConf
	class IFragmentConf : public QObject {
		Q_OBJECT

		// CONSTRUCTOR AND DESTRUCTOR
		public:
			IFragmentConf() :
				QObject(),
				mRelev(0.0f),
				mXF(illegal<XF>()),
				mCP(illegal<vec3>()),
				mCPRadius(illegal<float>()) {}
			virtual ~IFragmentConf() {}
			IFragmentConf(const IFragmentConf& that) :
				QObject(),
				mRelev(that.mRelev),
				mXF(that.mXF),
				mCP(that.mCP),
				mCPRadius(that.mCPRadius) {
				for (int i = 0; i < MAX_FRAGMENTS; ++i)
					mFragments[i] = that.mFragments[i];
			}
			virtual IFragmentConf& operator=(const IFragmentConf& that) {
				mRelev = that.mRelev;
				mXF = that.mXF;
				mCP = that.mCP;
				mCPRadius = that.mCPRadius;

				for (int i = 0; i < MAX_FRAGMENTS; ++i)
					mFragments[i] = that.mFragments[i];

				return *this;
			}; // leave it to the compiler, for now...

		// FIELDS
		public:
			enum {
				TARGET,
				SOURCE,
				MAX_FRAGMENTS
			};

			typedef enum {
				RELEVANCE = 0x0001,
				TARGET_FRAGMENT = 0x0002,
				SOURCE_FRAGMENT = 0x0004,
				TRANSFORMATION = 0x0008,
				CONTOUR_POINT = 0x0010,
				CONTOUR_POINT_RADIUS = 0x0020,
				ALL = 0xffffffff
			} Signature;

			// When adding new fields, don't forget updating lessAllFields() !!!
			//
			Fuzzy  mRelev;                     //!< Relevance of this match.
			int    mFragments[MAX_FRAGMENTS];  //!< Fragment indices involved.
			XF     mXF;                        //!< Transformation matching source to target.
			vec3   mCP;                        //!< Closest point (the match is supposed to happen near this point).
			float  mCPRadius;                  //!< Radius of influence around mCP.

		// METHODS
		public:
			virtual int index() const { return 1; } // as far as I can see this is only important for metadata purposes (Nicolas)
			virtual void updateID(int id) = 0;

			virtual bool setMetaData(const QString &field, const QString &value) const = 0;
			virtual bool setMetaData(const QString &field, double value) const = 0;
			virtual QString getString(const QString &field, const QString &deflt="") const = 0;
			virtual double getDouble(const QString &field, double deflt=0.0) const = 0;

			virtual const QString getTargetId() const { return Database::entryID(mFragments[TARGET]); }
			virtual const QString getSourceId() const { return Database::entryID(mFragments[SOURCE]); }

			virtual unsigned int signature(void) const {
				unsigned int result = 0;
				if (!isIllegal(mRelev))  result |= RELEVANCE;
				if (!isIllegal(mXF))  result |= TRANSFORMATION;
				if (!isIllegal(mCP))  result |= CONTOUR_POINT;
				if (!isIllegal(mCPRadius))  result |= CONTOUR_POINT_RADIUS;
				if (mFragments[TARGET] >= 0)  result |= TARGET_FRAGMENT;
				if (mFragments[SOURCE] >= 0)  result |= SOURCE_FRAGMENT;
				return result;
			}

			// Checks for superset of attributes.
			virtual bool signatureContains(unsigned signature) const {
		        unsigned  mySig = this->signature();
		        return (signature & mySig) == signature;
		    }

			// Checks for equality with respect to attributes in \a mask;
			virtual bool signatureMatches(unsigned signature, unsigned mask=ALL) const {
				return (this->signature() & mask) == signature;
			}


			// Swaps source and target fragment in this configuration.
			// Their transformation is adjusted appropriately.
			// Use this function to play safe with respect to future extensions.
			virtual void swapSourceAndTarget(void) {
		        std::swap(mFragments[SOURCE], mFragments[TARGET]);
		        mXF = inv(mXF);
		    }

			virtual void print(const char *file = NULL, int line = -1) const {
				char buf[1024];

				if (file) {
					sprintf(buf, "%s:%d: ", file, line);
				}
				else {
					buf[0] = 0;
				}

				qDebug(
					"%s[%d,%d], relev: %g, XF[0]: %g, cp: %g,%g,%g, cpr: %g\n",
					buf,
					mFragments[0], mFragments[1], mRelev.toFloat(), mXF[0], mCP[0], mCP[1], mCP[2], mCPRadius
				);
			}
	};
}

			/*
			static const QStringList& metaDataOrderedFieldNames(void) { return mMetaDataOrderedFieldNames; }
			static MetaDataField  *addMetaDataField(const QString &name, MetaDataField::Type type, const MetaDataField::Description &description);
			static MetaDataField *metaDataField(const QString &field) {
			  QHash<QString,MetaDataField*>::const_iterator  it(mMetaData.find(field));
			  if (it == mMetaData.end())
				return NULL;
			  else
				return *it;
			}
			*/

			/*
			virtual friend FragmentConf operator&&(FragmentConf h1, FragmentConf h2) = 0;
			virtual friend FragmentConf operator||(FragmentConf h1, FragmentConf h2) = 0;
			virtual friend bool operator<(FragmentConf h1, FragmentConf h2) = 0;
			virtual friend bool lessAllFields(FragmentConf h1, FragmentConf h2) = 0;
			virtual friend bool  lessFragmentIDs(FragmentConf h1, FragmentConf h2) = 0;
			virtual friend bool  lessRelevance(FragmentConf h1, FragmentConf h2) = 0;
			*/

#endif /* IFRAGMENTCONF_H_ */
