#ifndef ATTRIBUTEMERGEITEM_H_
#define ATTRIBUTEMERGEITEM_H_

#include "MergeItem.h"
#include "MergeAction.h"

#include <QList>
#include <QDebug>
#include <QTableWidget>

#include "SQLRawTheraRecords.h"

class AttributeMergeItem : public MergeItem {
	public:
		AttributeMergeItem(int matchId, const QList<HistoryRecord>& masterHistory, const QList<HistoryRecord>& slaveHistory, const QString& attributeName);

		virtual MergeItemType type() const { return ATTRIBUTEMERGEITEM; }
		virtual QString typeString() const { return "Attribute merge"; }

		virtual QString message() const {
			QString message;

			if (!isDone()) {
				message = mMessage;
			}
			else {

			}

			return message;
		}

		// TODO: replace by static QList so it doesn't need to be constructed all the time
		virtual QList<Merge::Action> acceptedActions() const {
			return QList<Merge::Action>() << Merge::CHOOSE_SLAVE << Merge::MOST_RECENT << Merge::DONT_MERGE << Merge::CHOOSE_HISTORY;
		}

		virtual void accept(const SimpleMergeAction *action);
		virtual bool execute(SQLDatabase *db, MergeMapper *mapper);

		virtual QWidget *informationWidget() const;

	private:
		inline bool isValidId(int id) const { return id != -1; }

		MergeAction *chooseHistory();
		MergeAction *chooseMostRecent();
		MergeAction *chooseSlave();

		MergeAction *decideForInvalidHistories();

		QTableWidget *createTableWidget(const QList<HistoryRecord>& history) const;

	private:
		int mMatchId;

		const QList<HistoryRecord> mMasterHistory;
		const QList<HistoryRecord> mSlaveHistory;

		QString mMessage;
		QString mAttributeName;

		bool mMergeIn;
};

#endif /* ATTRIBUTEMERGEITEM_H_ */
