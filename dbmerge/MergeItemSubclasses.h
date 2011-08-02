#ifndef MERGEITEMSUBCLASSES_H_
#define MERGEITEMSUBCLASSES_H_

#include "MergeItem.h"
#include "MergeAction.h"

#include <QList>
#include <QDebug>

class MatchMergeItem : public MergeItem {
	public:
		MatchMergeItem(int oldId, const QString& sourceId, const QString& targetId, const thera::XF& xf)
			: mOldId(oldId), mNewId(-1), mSourceId(sourceId), mTargetId(targetId), mXF(xf) { }

		virtual MergeItemType type() const { return MATCHMERGEITEM; }
		virtual QString typeString() const { return "Match merge"; };

		virtual QString message() const {
			QString message;

			if (!isDone()) {
				if (mNewId == mOldId && mOldId != -1) {
					message = QString("Id %3 didn't exist in master and no similar match to %1 <-> %2 was found").arg(mSourceId).arg(mTargetId).arg(mOldId);
				}
				else {
					message = QString("Id %3 already existed in master but no similar match to %1 <-> %2 was found").arg(mSourceId).arg(mTargetId).arg(mOldId);
				}
			}
			else {
				message = QString("Done, old ID %1, new ID %2").arg(mOldId).arg(mNewId);
			}

			return message;
		}

		// TODO: replace by static QList so it doesn't need to be constructed all the time
		virtual QList<Merge::Action> acceptedActions() const {
			return QList<Merge::Action>() << Merge::ASSIGN_NEW_ID << Merge::DONT_MERGE;
		}

		virtual void accept(const SimpleMergeAction *action) {
			switch (action->type()) {
				case Merge::ASSIGN_NEW_ID: {
					const AssignIdAction *assignIdAction = static_cast<const AssignIdAction *>(action);

					mNewId = assignIdAction->assignId();
				} break;

				case Merge::DONT_MERGE: break;

				default:
					qDebug() << "MatchMergeItem::accept: received unacceptable action";
					return;
			}

			store(action);
		}

		virtual bool execute(SQLDatabase *db, MergeMapper *mapper) {
			if (currentActionType() == Merge::ASSIGN_NEW_ID) {
				const thera::SQLFragmentConf& c = (mNewId == -1) ? db->addMatch(mSourceId, mTargetId, mXF) : db->addMatch(mSourceId, mTargetId, mXF, mNewId);

				if (c.index() == -1) {
					qDebug() << "MatchMergeItem::execute: got an invalid fragment conf";

					return false;
				}

				if (mNewId == -1) {
					// TODO: technically superfluous since the mapper already displays something if this happens
					if (mapper->exists(MergeMapper::MATCH_ID, mOldId)) {
						qDebug() << "MatchMergeItem::execute: mapping" << MergeMapper::MATCH_ID << "->" << mOldId << "somehow already existed, this could lead to problems down the road";
					}

					mapper->addMapping(MergeMapper::MATCH_ID, mOldId, mNewId);

					mNewId = c.index();
				}
				else {
					if (mNewId != c.index()) {
						qDebug() << "MatchMergeItem::execute: returned id did not match with expected id: expected = " << mNewId << " and actual =" << c.index();

						mNewId = c.index();
					}
				}

				setDone(true);
			}

			return true;
		}

	private:
		int mOldId, mNewId;

		QString mSourceId, mTargetId;

		thera::XF mXF;
};

#endif /* MERGEITEMSUBCLASSES_H_ */
