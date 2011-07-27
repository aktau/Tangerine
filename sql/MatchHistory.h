#ifndef MATCHHISTORY_H_
#define MATCHHISTORY_H_

#include <QDateTime>
#include <QVariant>

struct HistoryRecord {
	int matchId;
	int userId;

	QDateTime timestamp;
	QVariant value;

	HistoryRecord(int _userId, int _matchId, const QDateTime& _timestamp, const QVariant _value)
		: matchId(_matchId), userId(_userId), timestamp(_timestamp), value(_value)  { }
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
