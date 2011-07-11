#ifndef THUMBLABEL_H_
#define THUMBLABEL_H_

#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QPixmapCache>
#include <QDir>

#include "IMatchModel.h"

class ThumbLabel : public QLabel {
		Q_OBJECT;

	public:
		ThumbLabel(int i, QWidget *parent = NULL) : QLabel(parent), idx(i), mSelected(false), mIsDuplicate(false), mStatus(IMatchModel::UNKNOWN) {
			// ensure 100 MB of cache
			QPixmapCache::setCacheLimit(102400);
		}

		/**
		 * keeps the old status
		 */
		void setThumbnail(const QString& file = QString()) {
			if (file.isEmpty()) {
				mStatus = IMatchModel::UNKNOWN;
				mIsDuplicate = false;
			}

			setThumbnail(file, mStatus, mIsDuplicate);
		}

		void setThumbnail(const QString& file, IMatchModel::Status status, bool isDuplicate = false) {
			mSource = file;
			mStatus = status;
			mSelected = false;
			mIsDuplicate = isDuplicate;

			paintThumbnail();
			paintStatus();
		}

		void setDuplicate(bool value) {
			mIsDuplicate = value;

			paintThumbnail();
			paintStatus();
		}

		void setStatus(IMatchModel::Status status) {
			mStatus = status;

			// ugly-ish hack for when the thumb is selected, in which case we would have to blend the selection marker afterwards
			unselect();
			paintStatus();
			select();
		}

		void select() {
			if (!isSelected()) {
				QPixmap p = QPixmap(pixmap()->width(), pixmap()->height());
				QPainter painter(&p);

				painter.drawPixmap(rect(), *pixmap());
				painter.fillRect(rect(), QColor(255,255,255,50));

				setPixmap(p);

				mSelected = true;
			}
		}

		void unselect() {
			if (isSelected()) {
				setThumbnail(mSource);

				mSelected = false;
			}
		}

		bool isSelected() const {
			return mSelected;
		}

	signals:
		void clicked(int i, QMouseEvent *event);
		void doubleClicked(int i, QMouseEvent *event);

	protected:
		virtual void mousePressEvent(QMouseEvent *event) { emit clicked(idx, event); }
		virtual void mouseDoubleClickEvent(QMouseEvent *event) { emit doubleClicked(idx, event); }

		void paintStatus() {
			QPixmap p = *pixmap();
			QPainter painter(&p);

			QColor c;
			switch (mStatus) {
				case IMatchModel::UNKNOWN: c = Qt::black; break; // unknown
				case IMatchModel::YES: c = Qt::green; break; // correct
				case IMatchModel::MAYBE: c = QColor(255, 128, 0); break; // maybe
				case IMatchModel::NO: c = Qt::red; break; // no
				case IMatchModel::CONFLICT: c = /* Qt::magenta */ QColor(128, 128, 128); break; // no by conflict

				default: {
					qDebug() << "ThumbLabel::setStatus: encountered unknown kind of status. Thumb" << idx << "- Status" << mStatus;

					c = Qt::white;
				}
			};

			painter.fillRect(0, 0, width(), 10, c);

			setPixmap(p);
		}

		void paintThumbnail() {
			const bool exists = QFile::exists(mSource);
			const bool empty = mSource.isEmpty();

			QString cachedSource;

			if (exists) cachedSource = mSource;
			else if (empty) cachedSource = ".empty";
			else cachedSource = ".invalid";

			QPixmap p;

			if (!QPixmapCache::find(cachedSource, &p)) {
				p = !exists ?  QPixmap(width(), height()) : QPixmap(cachedSource);

				if (empty) {
					p.fill(Qt::black);
				}
				else if (!exists) {
					p.fill(Qt::lightGray);
				}
				else {
					p = p.scaledToWidth(width(), Qt::SmoothTransformation);
				}

				QPixmapCache::insert(mSource, p);
			}

			if (!mIsDuplicate) {
				setPixmap(p);
			}
			else {
				QPixmap final = *pixmap();
				final.fill(Qt::black);
				QPainter painter(&final);

				const int statusOffset = 10;
				const int spare = 15;
				const int layerMaxWidth = width() - spare;
				const int layerMaxHeight = height() - spare;
				const int numlayers = 3;
				const int spacePerLayer = spare / numlayers;

				for (int i = numlayers; i > 0; --i) {
					const int greyval = 60 + (160 - 60) / i;
					QColor c(greyval, greyval, greyval);

					const int offsetFromBorder = spacePerLayer * (numlayers + 1 - i);

					// bottom row
					painter.fillRect(spacePerLayer * i, height() - offsetFromBorder, layerMaxWidth, spacePerLayer, c);

					// right column
					painter.fillRect(width() - offsetFromBorder, statusOffset + spacePerLayer * i, spacePerLayer, layerMaxHeight - statusOffset, c);
				}

				p = p.scaled(width() - spare, height() - spare, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				painter.drawPixmap(0,0,p);

				setPixmap(final);

			}
		}

	public:
		const int idx;

	private:
		bool mSelected;
		bool mIsDuplicate;

		QString mSource;

		IMatchModel::Status mStatus;
};

#endif /* THUMBLABEL_H_ */
