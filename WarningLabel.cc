#include "WarningLabel.h"

#include <QtGui>

#include <QDebug>

WarningLabel::WarningLabel(const QString& title, const QString& text, QWidget *parent, Qt::WindowFlags f)
	: QWidget(parent, f), mTitle(title), mText(text), mDuration(3333), mEndOpacity(0.0) {
	setAttribute(Qt::WA_DeleteOnClose, true);

	setStyleSheet("QWidget { background-color: black; } QLabel { color : white; }");
	mLabel = new QLabel();
	mLabel->setAlignment(Qt::AlignCenter);
	mLabel->setTextFormat(Qt::RichText);
	mLabel->setText(QString("<h1>%1</h1><p>%2</p><small>click this message to hide it</small>").arg(mTitle).arg(mText));

	mEffect = new QGraphicsOpacityEffect(this);
	setGraphicsEffect(mEffect);

	mAnim = new QPropertyAnimation(mEffect, "opacity");
	connect(mAnim, SIGNAL(finished()), this, SLOT(finished()));

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget(mLabel);
	setLayout(hbox);

	//resize(parent->size() / 2);
	QSize size =  parent->size() / 2;
	QPoint topLeft = parent->rect().center();
	topLeft.rx() -= size.rwidth() / 2;
	topLeft.ry() -= size.rheight() / 2;

	setGeometry(QRect(topLeft, size));
}

WarningLabel::~WarningLabel() { }

void WarningLabel::setOneLine() {
	mLabel->setText(QString("<b>%1</b>: %2 (click this message to hide it)").arg(mTitle).arg(mText));
}

void WarningLabel::setEndOpacity(float endOpacity) {
	mEndOpacity = endOpacity;
}

void WarningLabel::fade() {
	mAnim->setStartValue(1.0);
	mAnim->setEndValue(mEndOpacity);
	mAnim->setDuration(mDuration);
	mAnim->start();
}

void WarningLabel::finished() {
	if (mEffect->opacity() == 0) {
		close();
	}
}

void WarningLabel::mousePressEvent(QMouseEvent *) {
	close();
}
