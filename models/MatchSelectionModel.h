#ifndef MATCHSELECTIONMODEL_H_
#define MATCHSELECTIONMODEL_H_

#include <QObject>
#include <QItemSelectionModel>
#include <QList>
#include <QSet>

#include "IMatchModel.h"

/**
 * Represents the selected items in a model
 *
 * (we don't inherit from QItemSelectionModel because our model indices are plain int's and
 * not QModelIndex'es, we might change this in the future and that's why we base our interace
 * on that of QItemSelectionModel, loosely.
 */
class MatchSelectionModel : public QObject {
		Q_OBJECT

	public:
		MatchSelectionModel(IMatchModel *model, QObject *parent = NULL);
		virtual ~MatchSelectionModel();

	public:
		virtual QList<int> selectedIndexes() const;

	public slots:
		virtual void select(int index, QItemSelectionModel::SelectionFlags command);
		virtual void select(const QList<int>& selection, QItemSelectionModel::SelectionFlags command);
		virtual void setCurrentIndex(int index, QItemSelectionModel::SelectionFlags command = QItemSelectionModel::Current);
		virtual void clearSelection();

	signals:
		void selectionChanged(const QList<int>& selected, const QList<int>& deselection);
		void currentChanged(int current, int previous);

	private:
		int mCurrent;

		QSet<int> mSelection;

		IMatchModel *mModel;
};

#endif /* MATCHSELECTIONMODEL_H_ */
