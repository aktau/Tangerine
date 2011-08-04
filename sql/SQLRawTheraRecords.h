#ifndef MATCHHISTORY_H_
#define MATCHHISTORY_H_

#include <QDateTime>
#include <QVariant>

/**
 * These classes are meant for the times when intense modification of the database is necessary, most
 * of this functionality is exposed through the SQLFragmentConf interface but when dealing with huge
 * amounts of matches it might in some cases be more performant/easier to request the records themselves
 */

struct AttributeRecord {
	int matchId;

	QVariant value;
};

// TODO: use an AttributeRecord internally
struct HistoryRecord {
	int matchId;
	int userId;

	QDateTime timestamp;
	QVariant value;

	HistoryRecord(int _userId, int _matchId, const QDateTime& _timestamp, const QVariant _value)
		: matchId(_matchId), userId(_userId), timestamp(_timestamp), value(_value)  { }

	QString toString() const {
		return QString("Match: %1, user: %2, value: %3 @ date: %4").arg(matchId).arg(userId).arg(value.toString()).arg(timestamp.toString());
	}

	bool operator==(const HistoryRecord& other) const {
		return timestamp == other.timestamp && matchId == other.matchId && userId == other.userId && value == other.value;
	}
};

/**
 * None of the methods in this class will be declared virtual,
 * since the only planned extension is a superclass like IMatchHistory (an interface)
 * in which case that interface could declare all necessary functions to be (pure) virtual
 * and it would cascade down
 */

// TODO: FieldHistory!!!
/*
class MatchHistory {
	public:
		MatchHistory(int index);
		virtual ~MatchHistory();

	public:
		int matchIndex() const;

		QList<HistoryRecord> getRecords(const QString &field) const;

	private:
		int mIndex;

		SQLDatabase *mDb;

		QMap<QString, QList<HistoryRecord> > *mCachedValues;
};
*/

#endif /* MATCHHISTORY_H_ */
