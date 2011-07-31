#ifndef MERGETABLEWIDGET_H_
#define MERGETABLEWIDGET_H_

#include <QTableWidget>

class MergeTableWidget : public QTableWidget {
		Q_OBJECT

	public:
		MergeTableWidget(QWidget *parent = 0);
		MergeTableWidget(int rows, int columns, QWidget *parent = 0);
		virtual ~MergeTableWidget();

		int clickableColumn() const;
		void setClickableColumn(int column);

	protected:
		void mouseMoveEvent(QMouseEvent *event);

	private:
		void init();

	private:
		int mClickableColumn;
};

#endif /* MERGETABLEWIDGET_H_ */
