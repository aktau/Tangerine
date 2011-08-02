#ifndef WARNINGLABEL_H_
#define WARNINGLABEL_H_

#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

class WarningLabel : public QWidget {
		Q_OBJECT

	public:
		WarningLabel(const QString& title, const QString& text, QWidget *parent = 0, Qt::WindowFlags f = 0);
		virtual ~WarningLabel();

		void setOneLine();
		void setEndOpacity(float endOpacity = 0.0); // the ending opacity, default is 0.0
		void fade();

	protected:
		void mousePressEvent(QMouseEvent *event);

	private slots:
		void finished();

	private:
		QLabel *mLabel;
		QGraphicsOpacityEffect *mEffect;
		QPropertyAnimation *mAnim;

		QString mTitle;
		QString mText;

		int mDuration;
		float mEndOpacity;
};

#endif /* WARNINGLABEL_H_ */
