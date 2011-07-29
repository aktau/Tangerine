#ifndef MERGEMAPPER_H_
#define MERGEMAPPER_H_

#include <QDebug>
#include <QString>

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
		void addMapping(MergeMapper::IntField field, int from, int to) { qDebug() << "INTFIELD!!!" << field << from << to; }
		void addMapping(MergeMapper::StringField field, const QString& from, const QString& to) { qDebug() << "STRINGFIELD!!" << field << from << to; }
		//template<typename T> T map(int id, const T& from) const;

	private:
};

#endif /* MERGEMAPPER_H_ */
