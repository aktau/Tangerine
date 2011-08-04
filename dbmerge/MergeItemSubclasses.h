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
				if (isValidId(mNewId)) {
					message = QString("Done, slave ID %1, master ID %2").arg(mOldId).arg(mNewId);
				}
				else {
					message = QString("Done, did not add (slave) %1 to master").arg(mOldId);
				}
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
			bool success = true;

			if (currentActionType() == Merge::ASSIGN_NEW_ID) {
				const thera::SQLFragmentConf& c = (mNewId == -1) ? db->addMatch(mSourceId, mTargetId, mXF) : db->addMatch(mSourceId, mTargetId, mXF, mNewId);

				if (c.index() == -1) {
					qDebug() << "MatchMergeItem::execute: got an invalid fragment conf, match with old id" << mOldId << "definitely not added to the DB";

					success = false;
				}
				else {
					if (mNewId != -1 && mNewId != c.index()) {
						qDebug() << "MatchMergeItem::execute: returned id did not match with expected id: expected = " << mNewId << " and actual =" << c.index();
					}
				}

				// whatever c.index() we get, that's the new id. If it's -1 that means the pair wasn't correctly added for some reason
				mNewId = c.index();
			}
			else {
				// it should already be -1 but just making sure
				mNewId = -1;
			}

			// note that it is possible that:
			// 	mOldId == mNewId
			//  mNewId == -1
			// especially this last situation should be taken into account, it means that the item *wasn't* merged in
			mapper->addMapping(MergeMapper::MATCH_ID, mOldId, mNewId);

			setDone(true);

			return true;
		}

	private:
		inline bool isValidId(int id) const {
			return id != -1;
		}

	private:
		int mOldId, mNewId;

		QString mSourceId, mTargetId;

		thera::XF mXF;
};

/*
class AttributeMergeItem : public MergeItem {
	public:
		AttributeMergeItem() { }

		virtual MergeItemType type() const { return ATTRIBUTEMERGEITEM; }
		virtual QString typeString() const { return "Attribute merge"; };

		virtual QString message() const {
			QString message;

			if (!isDone()) {
			}
			else {
			}

			return message;
		}

		// TODO: replace by static QList so it doesn't need to be constructed all the time
		virtual QList<Merge::Action> acceptedActions() const {
			return QList<Merge::Action>() << Merge::CHOOSE_SLAVE << Merge::MOST_RECENT << Merge::DONT_MERGE;
		}

		virtual void accept(const SimpleMergeAction *action) {
			switch (action->type()) {
				case Merge::ASSIGN_NEW_ID: {
					const AssignIdAction *assignIdAction = static_cast<const AssignIdAction *>(action);
				} break;

				case Merge::DONT_MERGE: break;

				default:
					qDebug() << "MatchMergeItem::accept: received unacceptable action";
					return;
			}

			store(action);
		}

		virtual bool execute(SQLDatabase *db, MergeMapper *mapper) {
			bool success = true;

			if (currentActionType() == Merge::ASSIGN_NEW_ID) {

			}

			//mapper->addMapping(MergeMapper::MATCH_ID, mOldId, mNewId);

			setDone(true);

			return true;
		}

	private:
		inline bool isValidId(int id) const {
			return id != -1;
		}
};
*/

#endif /* MERGEITEMSUBCLASSES_H_ */
