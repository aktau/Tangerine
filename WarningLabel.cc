#include "WarningLabel.h"

#include <QtGui>

#include <QDebug>

#define QT_USE_FAST_CONCATENATION
#define QT_USE_FAST_OPERATOR_PLUS

WarningLabel::WarningLabel(const QString& title, const QString& text, WarningLabel::Position position, QWidget *parent, Qt::WindowFlags f)
	: QWidget(parent, f), mTitle(title), mText(text), mDuration(3333), mEndOpacity(0.5), mLinger(true) {
	init();

	setText(mTitle, mText);
	setPosition(position);
}

WarningLabel::WarningLabel(QWidget *parent, Qt::WindowFlags f)
	: QWidget(parent, f), mDuration(3333), mEndOpacity(0.5), mLinger(true) {
	init();
}

WarningLabel::~WarningLabel() {
	qDebug() << "WarningLabel::~WarningLabel:" << mTitle << "destroyed";
}

void WarningLabel::init() {
	//setAttribute(Qt::WA_DeleteOnClose, true);

	//setStyleSheet("QWidget { background-color: black; } QLabel { color : white; }");
	mLabel = new QLabel();
	mLabel->setAlignment(Qt::AlignCenter);
	mLabel->setTextFormat(Qt::RichText);

	setColors(Qt::black, Qt::white);

	mEffect = new QGraphicsOpacityEffect(this);
	setGraphicsEffect(mEffect);

	mAnim = new QPropertyAnimation(mEffect, "opacity");
	connect(mAnim, SIGNAL(finished()), this, SLOT(finished()));

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget(mLabel);
	setLayout(hbox);

	//show();
}

void WarningLabel::setColors(const QColor& background, const QColor& foreground) {
	const QString style = QString("QWidget { background-color: rgb(%1, %2, %3); } QLabel { color : rgb(%4, %5, %6); }")
		.arg(background.red()).arg(background.green()).arg(background.blue()).arg(foreground.red()).arg(foreground.green()).arg(foreground.blue());

	setStyleSheet(style);
}

void WarningLabel::setLinger(bool linger, float endOpacity) {
	setAttribute(Qt::WA_DeleteOnClose, !linger);

	mEndOpacity = endOpacity;
}

void WarningLabel::setBackground(const QColor& color) {
	if (color.isValid()) {
		mBackground = color;
		setColors(mBackground, mForeground);
	}
}

void WarningLabel::setForeground(const QColor& color) {
	if (color.isValid()) {
		mForeground = color;
		setColors(mBackground, mForeground);
	}
}

void WarningLabel::setPosition(WarningLabel::Position position) {
	QRect newPos = rect();

	QWidget *parent = parentWidget();

	if (parent) {
		switch (position) {
			case Center: {
				QSize size =  parent->size() / 2;
				QPoint topLeft = parent->rect().center();
				topLeft.rx() -= size.rwidth() / 2;
				topLeft.ry() -= size.rheight() / 2;

				newPos = QRect(topLeft, size);
			} break;

			case TopBarFull: {
				newPos = QRect(0, 0, parent->width(), 50);
			} break;

			default:
				qDebug() << " WarningLabel::setPosition: unknown position" << position;
		}

		//QRect geom = parent->geometry();
		//geom.setHeight(50);
		//move(0,0);
		//resize(parent->width(), 50);
		setGeometry(newPos);
		//setGeometry(geom);

		//qDebug() << "WarningLabel::setPosition: parent =" << parent->rect() << "parent geom =" << parent->geometry() << "and this geometry" << geometry() << "this rect =" << rect();
	}
}

void WarningLabel::setText(const QString& title, const QString& text, const QString& titleTag, const QString& textTag, bool oneLine) {
	mTitle = title;
	mText = text;

	const QString htmlTitle = (!titleTag.isEmpty()) ? QString("<%2>%1</%2>").arg(mTitle).arg(titleTag) : mTitle;
	const QString htmlText = (!textTag.isEmpty()) ? QString("<%2>%1</%2>").arg(mText).arg(textTag) : mText;

	mLabel->setText(htmlTitle + (oneLine ? QString(": ") : QString()) + htmlText + (oneLine ? QString() : "<small>click this message to hide it</small>"));
}

void WarningLabel::setOneLine() {
	setText(mTitle, mText, "b", QString(), true);
}

void WarningLabel::fade() {
	mAnim->setStartValue(1.0);
	mAnim->setEndValue(mEndOpacity);
	mAnim->setDuration(mDuration);
	mAnim->start();

	qDebug() << "animation started!";
}

void WarningLabel::finished() {
	if (!mLinger) {
		close();
	}

	//if (mEffect->opacity() == 0)
}

void WarningLabel::mousePressEvent(QMouseEvent *) {
	close();
}
