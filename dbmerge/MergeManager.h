#ifndef MERGEMANAGER_H_
#define MERGEMANAGER_H_

#include <QDialog>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QSharedPointer>
#include <QList>

#include "Merger.h"
#include "SQLDatabase.h"

class MergeManager : public QDialog {
		Q_OBJECT

	public:
		MergeManager(QWidget *parent = NULL);
		~MergeManager();

	public:
		// takes ownership of the merger, do NOT delete yourself
		void addMerger(Merger *merger);

		// will run all the added mergers in FIFO sequence
		void merge(QSharedPointer<SQLDatabase> left, QSharedPointer<SQLDatabase> right);

	private slots:
		void pickDatabases();

	private:
		QList<Merger *> mMergers;

		QTableWidget *mItemList;
		QDialogButtonBox *mButtonBox;
};

#endif /* MERGEMANAGER_H_ */
