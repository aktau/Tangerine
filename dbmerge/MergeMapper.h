#ifndef MERGEMAPPER_H_
#define MERGEMAPPER_H_

#include <QDebug>
#include <QString>
#include <QMap>

class MergeMapper {
	public:
		typedef enum {
			USER_ID,
			MATCH_ID,
			NUM_MAPPABLE_FIELDS
		} IntField;

		typedef enum {
			COMMENT,
			NUM_MAPPABLE_STRINGFIELDS
		} StringField;

	public:
		void addMapping(MergeMapper::IntField field, int from, int to) {
			// TODO: remove for performance
			if (exists(field, from)) qDebug() << "MergeMapper::addMapping (int): mapping already exists:" << field << from << to;

			mIntMap.insert(FieldIntPair(field,from), to);
		}

		void addMapping(MergeMapper::StringField field, const QString& from, const QString& to) {
			// TODO: remove for performance
			if (exists(field, from)) qDebug() << "MergeMapper::addMapping (string): mapping already exists:" << field << from << to;

			mStringMap.insert(FieldStringPair(field, from), to);
		}

		bool exists(MergeMapper::IntField field, int from) const {
			return mIntMap.contains(FieldIntPair(field, from));
		}

		bool exists(MergeMapper::StringField field, const QString& from) const {
			return mStringMap.contains(FieldStringPair(field, from));
		}

		// this variant will return the 'from' parameter as the result if nothing was found
		int get(MergeMapper::IntField field, int from) const {
			IntMap::const_iterator i = mIntMap.constFind(FieldIntPair(field, from));

			return (i != mIntMap.constEnd()) ? i.value() : from;
		}

		int get(MergeMapper::IntField field, int from, int deflt) const {
			IntMap::const_iterator i = mIntMap.constFind(FieldIntPair(field, from));

			return (i != mIntMap.constEnd()) ? i.value() : deflt;
		}

	private:
		typedef QPair<MergeMapper::IntField, int> FieldIntPair;
		typedef QMap<FieldIntPair, int> IntMap;
		IntMap mIntMap;

		typedef QPair<MergeMapper::StringField, QString> FieldStringPair;
		typedef QMap<FieldStringPair, QString> StringMap;
		StringMap mStringMap;
};

#endif /* MERGEMAPPER_H_ */
