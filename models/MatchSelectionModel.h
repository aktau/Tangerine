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
		virtual int currentIndex() const;
		virtual QList<int> selectedIndexes() const;
		virtual bool isSelected(int index) const;

	public slots:
		virtual void select(int index, QItemSelectionModel::SelectionFlags command);
		virtual void select(const QList<int>& selection, QItemSelectionModel::SelectionFlags command);
		virtual void setCurrentIndex(int index, QItemSelectionModel::SelectionFlags command = QItemSelectionModel::Current);
		virtual void clearSelection();

	signals:
		void selectionChanged(const QList<int>& selected, const QList<int>& deselected);
		void currentChanged(int current, int previous);

	protected:
		virtual void emitSelectionChanged(const QList<int>& newSelection, const QList<int>& oldSelection);
		virtual void emitSelectionChanged(const QSet<int>& newSelection, const QSet<int>& oldSelection);

		virtual void selectWithoutSignals(int index, QItemSelectionModel::SelectionFlags command, bool *selChanged, bool *idxChanged);

	private:
		int mCurrent;

		QSet<int> mSelection;

		IMatchModel *mModel;
};

#endif /* MATCHSELECTIONMODEL_H_ */
