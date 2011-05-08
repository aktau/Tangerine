#include "MatchSelectionModel.h"

MatchSelectionModel::MatchSelectionModel(IMatchModel *model, QObject *parent) : QObject(parent), mModel(model) {
	if (mModel == NULL) {
		qDebug() << "MatchSelectionModel::MatchSelectionModel: model was null";
	}
}

MatchSelectionModel::~MatchSelectionModel() {

}

QList<int> MatchSelectionModel::selectedIndexes() const {
	return QList<int>::fromSet(mSelection);
}

void MatchSelectionModel::select(int index, QItemSelectionModel::SelectionFlags command) {
	if (index < 0 || index >= mModel->size()) {
		qDebug() << "MatchSelectionModel::select: index out of range," << index << "of" << mModel->size();

		return;
	}

	switch (command) {
		case QItemSelectionModel::Select:
		{
			mSelection << index;
		}
		break;

		case QItemSelectionModel::Deselect:
		{
			mSelection.remove(index);
		}
		break;

		case QItemSelectionModel::Current:
		{
			setCurrentIndex(index);
		}
		break;

		case QItemSelectionModel::Clear:
		{
			clearSelection();
		}
		break;

		default:
			qDebug() << "MatchSelectionModel::select: unknown command" << command;
	}
}

void MatchSelectionModel::select(const QList<int>& selection, QItemSelectionModel::SelectionFlags command) {
	foreach (int index, selection) {
		select(index, command);
	}
}

void MatchSelectionModel::setCurrentIndex(int index, QItemSelectionModel::SelectionFlags command) {
	if (index < 0 || index >= mModel->size()) {
		qDebug() << "MatchSelectionModel::select: index out of range," << index << "of" << mModel->size();

		return;
	}

	mCurrent = index;
}

void MatchSelectionModel::clearSelection() {
	mSelection.clear();
}
