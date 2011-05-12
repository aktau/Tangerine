#include "MatchSelectionModel.h"

MatchSelectionModel::MatchSelectionModel(IMatchModel *model, QObject *parent) : QObject(parent), mCurrent(-1), mModel(model) {
	if (mModel == NULL) {
		qDebug() << "MatchSelectionModel::MatchSelectionModel: model was null, this is going to provoke a segfault down the line";
	}
	else {
		connect(mModel, SIGNAL(modelChanged()), this, SLOT(clearSelection()));
	}
}

MatchSelectionModel::~MatchSelectionModel() {

}

int MatchSelectionModel::currentIndex() const {
	return mCurrent;
}

QList<int> MatchSelectionModel::selectedIndexes() const {
	return QList<int>::fromSet(mSelection);
}

bool MatchSelectionModel::isSelected(int index) const {
	return mSelection.contains(index);
}

void MatchSelectionModel::select(int index, QItemSelectionModel::SelectionFlags command) {
	select(QList<int>() << index, command);
}

void MatchSelectionModel::select(const QList<int>& selection, QItemSelectionModel::SelectionFlags command) {
	QSet<int> oldSelection = mSelection;
	int oldIndex = mCurrent;

	bool selChanged = false;
	bool idxChanged = false;

	// we're moving the clear out here because otherwise we would clear every time inside of the loop (selectWithoutSignals),
	// erasing all of our hard work selecting stuff...
	if (command & QItemSelectionModel::Clear) {
		if (!mSelection.isEmpty()) {
			mSelection.clear();

			selChanged = true;
		}

		command &= ~QItemSelectionModel::Clear;

		// this doesn't look nice, low priority for now
		if (command & QItemSelectionModel::Current) {
			int newCurrent = (selection.isEmpty() || !mModel->isValidIndex(selection.last())) ? -1 : selection.last();

			if (mCurrent != newCurrent) {
				mCurrent = (command &  QItemSelectionModel::Select) ? newCurrent : -1;

				idxChanged = true;
			}

			command &= ~QItemSelectionModel::Current;
		}
	}

	if (!selection.isEmpty() && command != QItemSelectionModel::NoUpdate) {
		foreach (int index, selection) {
			selectWithoutSignals(index, command, &selChanged, &idxChanged);
		}
	}

	// send signals to slots
	if (selChanged) {
		emitSelectionChanged(mSelection, oldSelection);
	}
	if (idxChanged) {
		emit currentChanged(mCurrent, oldIndex);
	}
}

void MatchSelectionModel::selectWithoutSignals(int index, QItemSelectionModel::SelectionFlags command, bool *selChanged, bool *idxChanged) {
	if (!mModel->isValidIndex(index) && command != QItemSelectionModel::Clear) {
		qDebug() << "MatchSelectionModel::selectWithoutSignals: index out of range," << index << "of" << mModel->size();

		return;
	}

	// the way it stands now, this is a dead code path (see select()...), but it can't really hurt...
	/*
	if (command & QItemSelectionModel::Clear && !mSelection.isEmpty()) {
		mSelection.clear();

		if (command & QItemSelectionModel::Current) {
			mCurrent = -1;

			*idxChanged = true;
		}

		*selChanged = true;
	}
	*/

	if (command & QItemSelectionModel::Select && !mSelection.contains(index)) {
		mSelection << index;

		*selChanged = true;
	}
	else if (command & QItemSelectionModel::Deselect && mSelection.contains(index)) {
		mSelection.remove(index);

		*selChanged = true;
	}

	if (command & QItemSelectionModel::Current) {
		if (mCurrent != index) {
			mCurrent = index;

			*idxChanged = true;
		}
	}
}

void MatchSelectionModel::setCurrentIndex(int index, QItemSelectionModel::SelectionFlags command) {
	select(index, command | QItemSelectionModel::Current);
}

void MatchSelectionModel::clearSelection() {
	select(0, QItemSelectionModel::Clear | QItemSelectionModel::Current);
}

void MatchSelectionModel::emitSelectionChanged(const QList<int>& newSelection, const QList<int>& oldSelection) {
	emitSelectionChanged(newSelection.toSet(), oldSelection.toSet());
}

void MatchSelectionModel::emitSelectionChanged(const QSet<int>& newSelection, const QSet<int>& oldSelection) {
	// the copies are unfortunately necessary, but this is not really a performance critical function
	emit selectionChanged(QSet<int>(newSelection).subtract(oldSelection).toList(), QSet<int>(oldSelection).subtract(newSelection).toList());
}
