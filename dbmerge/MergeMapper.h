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
			qDebug() << "INTFIELD!!!" << field << from << to;

			mIntMap.insert(FieldIntPair(field,from), to);
		}

		void addMapping(MergeMapper::StringField field, const QString& from, const QString& to) {
			qDebug() << "STRINGFIELD!!" << field << from << to;

			mStringMap.insert(FieldStringPair(field, from), to);
		}

		bool exists(MergeMapper::IntField field, int from) const {
			return mIntMap.contains(FieldIntPair(field, from));
		}

		bool exists(MergeMapper::StringField field, const QString& from) const {
			return mStringMap.contains(FieldStringPair(field, from));
		}

		//template<typename T> T map(int id, const T& from) const;

	private:
		typedef QPair<MergeMapper::IntField, int> FieldIntPair;
		typedef QMap<FieldIntPair, int> IntMap;
		IntMap mIntMap;

		typedef QPair<MergeMapper::StringField, QString> FieldStringPair;
		typedef QMap<FieldStringPair, QString> StringMap;
		StringMap mStringMap;
};

#endif /* MERGEMAPPER_H_ */
