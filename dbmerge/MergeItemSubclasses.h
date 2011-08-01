#ifndef MERGEITEMSUBCLASSES_H_
#define MERGEITEMSUBCLASSES_H_

#include "MergeItem.h"
#include "MergeAction.h"

#include <QList>
#include <QDebug>

class MatchMergeItem : public MergeItem {
	public:
		MatchMergeItem(const QString& sourceId, const QString& targetId, const QString& xfString)
			: mSourceId(sourceId), mTargetId(targetId), mXFString(xfString) { }

		virtual QString message() const {
			return QString("Match ID conflict, ID already existed but match not the same: %1 <-> %2").arg(mSourceId).arg(mTargetId);
		}

		// TODO: replace by static QList so it doesn't need to be constructed all the time
		virtual QList<Merge::Action> acceptedActions() const {
			return QList<Merge::Action>() << Merge::ASSIGN_NEW_ID << Merge::DONT_MERGE;
		}

		virtual void accept(const SimpleMergeAction *action) {
			switch (action->type()) {
				case Merge::ASSIGN_NEW_ID: break;
				case Merge::DONT_MERGE: break;

				default:
					qDebug() << "MatchMergeItem::accept: received unacceptable action";
					return;
			}

			store(action);
		}

		virtual QString getQuery() const {
			if (currentActionType() == Merge::ASSIGN_NEW_ID) {
				return QString(
					"INSERT INTO matches (source_id, source_name, target_id, target_name, transformation) "
					"VALUES (0, %1, 0, %2, %3)"
				).arg(mSourceId).arg(mTargetId).arg(mXFString);
			}
			else {
				return QString();
			}
		}

	private:
		QString mSourceId, mTargetId, mXFString;
};

#endif /* MERGEITEMSUBCLASSES_H_ */
