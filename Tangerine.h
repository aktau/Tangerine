#ifndef TANGERINE_H_
#define TANGERINE_H_

#include <QMainWindow>
#include <QScrollArea>
#include <QFrame>

#define DEV_PHASE "Alpha"
#define MAJ_VERSION 0
#define MIN_VERSION 1

class Tangerine : public QMainWindow {
	Q_OBJECT

	public:
		Tangerine(QWidget *parent = 0);
		virtual ~Tangerine();

	private:
		void setupWindow();

	private:
		QFrame *mFrame;
		QScrollArea *mScrollArea;

		static const int MIN_WIDTH;
		static const int MIN_HEIGHT;
};

#endif /* TANGERINE_H_ */
