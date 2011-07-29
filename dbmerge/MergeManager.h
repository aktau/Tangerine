#ifndef MERGEMANAGER_H_
#define MERGEMANAGER_H_

#include <QDialog>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QSharedPointer>
#include <QList>

#include "Merger.h"
#include "MergeMapper.h"
#include "SQLDatabase.h"

class MergeManager : public QDialog {
		Q_OBJECT

	public:
		MergeManager(QWidget *parent = NULL, QSharedPointer<SQLDatabase> master = QSharedPointer<SQLDatabase>());
		~MergeManager();

	public:
		// takes ownership of the merger, do NOT delete yourself
		void addMerger(Merger *merger);

	protected:
		// will run all the added mergers in FIFO sequence
		void merge(QSharedPointer<SQLDatabase> left, QSharedPointer<SQLDatabase> right);

		void addItemsToTable(const QList<MergeItem *>& items);

	private slots:
		void pickDatabases();

	private:
		// widgets
		QTableWidget *mItemList;
		QDialogButtonBox *mButtonBox;

		// merge fields
		QSharedPointer<SQLDatabase> mMaster;
		QList<Merger *> mMergers;

		MergeMapper mMapper;
};

#endif /* MERGEMANAGER_H_ */
