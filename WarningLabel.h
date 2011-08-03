#ifndef WARNINGLABEL_H_
#define WARNINGLABEL_H_

#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

class WarningLabel : public QWidget {
		Q_OBJECT

	public:
		typedef enum {
			Center,
			TopBarFull
		} Position;

	public:
		WarningLabel(const QString& title, const QString& text, WarningLabel::Position position = Center, QWidget *parent = 0, Qt::WindowFlags f = 0);
		WarningLabel(QWidget *parent = 0, Qt::WindowFlags f = 0);
		virtual ~WarningLabel();

		// if linger is true, the widget will not destroy itself when the animation is finished or it is clicked
		// if linger is false, the widget will stay at the endOpacity until clicked, and will not be destroyed even when clicked
		void setLinger(bool linger = true, float endOpacity = 0.5);

		void setPosition(WarningLabel::Position position);
		void setText(const QString& title, const QString& text, const QString& titleTag = "h1", const QString& textTag = "p", bool oneLine = false);
		void setOneLine();
		//void setEndOpacity(float endOpacity = 0.0); // the ending opacity, default is 0.0
		void fade();

	protected:
		void mousePressEvent(QMouseEvent *event);

	private:
		void init();

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

		bool mLinger;
};

#endif /* WARNINGLABEL_H_ */
