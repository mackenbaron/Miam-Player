#include "tabbar.h"
#include "settings.h"
#include "playlist.h"
#include "../treeview.h"

#include <QtDebug>

#include <QIcon>

TabBar::TabBar(TabPlaylist *parent) :
	QTabBar(parent), lineEdit(new QLineEdit(this)), tabPlaylist(parent)
{
	this->addTab(QString());
	this->setTabsClosable(true);
	this->setAcceptDrops(true);
	this->setDocumentMode(true);
	QWidget *w = this->tabButton(count() - 1, RightSide);
	this->setTabButton(count()-1, RightSide, NULL);
	this->setUsesScrollButtons(false);

	lineEdit->setVisible(false);
	lineEdit->setAlignment(Qt::AlignCenter);
	lineEdit->setFrame(false);
	lineEdit->installEventFilter(this);
	lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

	_timer = new QTimer(this);
	_timer->setInterval(300);
	_timer->setSingleShot(true);

	connect(lineEdit, &QLineEdit::returnPressed, this, &TabBar::renameTab);
	// Switch between tabs if the Drag & Drop isn't finished
	connect(_timer, &QTimer::timeout, [=]() {
		this->setCurrentIndex(tabAt(_targetRect.center()));
		_targetRect = tabRect(currentIndex());
	});

	connect(Settings::getInstance(), &Settings::fontHasChanged, [=](Settings::FontFamily ff, const QFont &newFont) {
		if (ff == Settings::PLAYLIST) {
			QFont font = newFont;
			font.setPointSizeF(font.pointSizeF() * 0.8);
			this->setFont(font);
			int h = fontMetrics().height() * 1.25;
			if (h >= 30) {
				this->setMinimumHeight(h);
				this->setMaximumHeight(h);
			} else {
				this->setMinimumHeight(30);
				this->setMaximumHeight(30);
			}
		}
	});
	w->deleteLater();
}

/** Redefined to return a square for the last tab which is the [+] button. */
QSize TabBar::tabSizeHint(int index) const
{
	if (index == count() - 1) {
		return QSize(height(), height());
	} else {
		//return QTabBar::tabSizeHint(index);
		QSize s = QTabBar::tabSizeHint(index);
		if (!Settings::getInstance()->isRectTabs()) {
			s.setWidth(s.width() + Settings::getInstance()->tabsOverlappingLength() * 2);
		}
		return s;
	}
}

/** Redefined to validate new tab name if the focus is lost. */
bool TabBar::eventFilter(QObject *obj, QEvent *event)
{
	// Accept the escape key
	if (obj == lineEdit && event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->key() == Qt::Key_Escape) {
			lineEdit->close();
		}
	} else if (obj == lineEdit && event->type() == QEvent::FocusOut) {
		renameTab();
	}
	return false;
}

/** Redefined to accept D&D from another playlist or the library. */
void TabBar::dropEvent(QDropEvent *event)
{
	int tab = this->tabAt(event->pos());
	if (Playlist *origin = qobject_cast<Playlist*>(event->source())) {
		Playlist *target;
		// Tracks were dropped on the [+] button
		if (tab == this->count() - 1) {
			target = tabPlaylist->addPlaylist();
		} else {
			target = tabPlaylist->playlist(tab);
		}
		// Copy tracks in the target
		QList<QMediaContent> medias;
		foreach (QPersistentModelIndex index, origin->selectionModel()->selectedRows()) {
			medias.append(origin->mediaPlaylist()->media(index.row()));
		}
		// Append tracks at the end
		target->insertMedias(target->model()->rowCount(), medias);

		// Remove tracks from the current playlist if necessary
		if (!Settings::getInstance()->copyTracksFromPlaylist()) {
			origin->removeSelectedTracks();
		}
	} else if (TreeView *origin = qobject_cast<TreeView*>(event->source())) {
		// Tracks were dropped on the [+] button
		if (tab == this->count() - 1) {
			tabPlaylist->addPlaylist();
		} else {
			tabPlaylist->setCurrentIndex(tab);
		}
		origin->appendToPlaylist();
	}
}

/** Redefined to accept D&D from another playlist or the library. */
void TabBar::dragEnterEvent(QDragEnterEvent *event)
{
	int idx = tabAt(event->pos());
	if (idx < count() - 1) {
		_targetRect = tabRect(idx);
	} else {
		_targetRect = QRect();
	}
	event->source() ? event->acceptProposedAction() : event->ignore();
}

/** Redefined to accept D&D from another playlist or the library. */
void TabBar::dragMoveEvent(QDragMoveEvent *event)
{
	int idx = tabAt(event->pos());
	// Exclude current tab and last one
	if (idx < count() - 1) {
		_targetRect = tabRect(idx);
	}
	Playlist *playlist = qobject_cast<Playlist*>(event->source());
	if (playlist) {
		// Exclude current tab and last one?
		if (idx < count() - 1) {
			_timer->start();
		}
	}
	event->source() ? event->acceptProposedAction() : event->ignore();
}

/** Redefined to display an editable area. */
void TabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
	int tabIndex = tabAt(event->pos());
	int c = currentIndex();
	if (-1 < tabIndex && tabIndex < count()-1 && c == tabIndex) {
		QRect visualRect = tabRect(tabIndex);
		visualRect.setLeft(visualRect.left() + 1);
		visualRect.setRight(visualRect.right() - 1);
		visualRect.setTop(visualRect.top() + 1);

		// Disable close buttons in case of unfortunate click
		for (int t = 0; t < count(); t++) {
			QWidget *button = tabButton(t, QTabBar::RightSide);
			if (button && t != tabIndex) {
				button->setEnabled(false);
			}
		}

		// Feed the QLineEdit with current tab text
		lineEdit->setText(tabText(tabIndex));
		lineEdit->selectAll();
		lineEdit->setFocus();
		lineEdit->setGeometry(visualRect);
		lineEdit->setVisible(true);
	}
	QTabBar::mouseDoubleClickEvent(event);
}

/** Redefined to validate new tab name without pressing return. */
void TabBar::mousePressEvent(QMouseEvent *event)
{
	int tabIndex = tabAt(event->pos());
	if (lineEdit->isVisible() && ((currentIndex() != tabIndex && lineEdit->isVisible()) || !lineEdit->geometry().contains(event->pos()))) {
		lineEdit->close();
	} else {
		QTabBar::mousePressEvent(event);
	}
}

void TabBar::paintEvent(QPaintEvent *)
{
	QStylePainter p(this);
	QStyleOptionTab o;

	int dist = Settings::getInstance()->tabsOverlappingLength();

	if (Settings::getInstance()->isRectTabs()) {
		paintRectTabs(p, o);
	} else {
		paintRoundedTabs(p, o, dist);
	}

	// Global bottom frame border
	int h = tabRect(0).height() - 1;
	p.setPen(o.palette.mid().color());
	if (count() > 2) {
		if (currentIndex() == 0) {
			p.drawLine(tabRect(0).right() + 1 + dist, h, rect().right(), h);
		} else {
			p.drawLine(rect().left(), h, tabRect(currentIndex()).left(), h);
			p.drawLine(tabRect(currentIndex()).right() + 1 + dist, h, rect().right(), h);
		}
	} else {
		if (isLeftToRight()) {
			p.drawLine(tabRect(0).width() + dist, h, rect().right(), h);
		} else {
			p.drawLine(rect().left(), h, tabRect(0).left(), h);
		}
	}
}

void TabBar::paintRectTabs(QStylePainter &p, QStyleOptionTab &o)
{
	// A "[+]" button on the right
	static const QPointF plus[13] = {
		QPointF(1, 2), QPointF(2, 2), QPointF(2, 1),
		QPointF(3, 1), QPointF(3, 2), QPointF(4, 2),
		QPointF(4, 3), QPointF(3, 3), QPointF(3, 4),
		QPointF(2, 4), QPointF(2, 3), QPointF(1, 3),
		QPointF(1, 2),
	};

	static const qreal penScaleFactor = 0.2;

	for (int i = 0; i < count(); i++) {
		initStyleOption(&o, i);

		// Background color
		p.save();
		if (i != currentIndex() && i != count() - 1) {
			o.rect.adjust(0, 2, 0, 0);
		} else if (i == count() - 1) {
			o.rect.adjust(2, 2, -4, -4);
		}

		// Highlight the tab under the cursor
		if (o.state.testFlag(QStyle::State_MouseOver) && i != currentIndex()) {
			p.setPen(QPen(o.palette.highlight(), penScaleFactor));
			p.fillRect(o.rect, o.palette.highlight().color().lighter());
		} else {
			p.setPen(QPen(o.palette.mid(), penScaleFactor));
			if (i == currentIndex()) {
				/// XXX
				if (Settings::getInstance()->isCustomColors()) {
					p.fillRect(o.rect, o.palette.base().color().lighter(110));
				} else {
					p.fillRect(o.rect, o.palette.base());
				}
			} else {
				p.fillRect(o.rect, o.palette.window());
			}
		}

		// Draw last tab frame (which is the [+] button)
		if (i == count() - 1) {
			QPen plusPen;
			if (o.state.testFlag(QStyle::State_MouseOver)) {
				plusPen = QPen(o.palette.highlight(), penScaleFactor);
				p.setPen(o.palette.highlight().color());
				p.setBrush(o.palette.highlight().color().lighter());
			} else {
				plusPen = QPen(o.palette.mid(), penScaleFactor);
				p.setPen(o.palette.mid().color());
				p.setBrush(o.palette.base());
			}
			p.drawRect(o.rect);

			p.translate(o.rect.topLeft());
			plusPen.setJoinStyle(Qt::MiterJoin);
			p.setPen(plusPen);

			// When the tabbar is very big, the inner color of [+] is a gradient like star ratings
			// Should I disable this gradient when height is small?
			p.scale(o.rect.height() * penScaleFactor, o.rect.height() * penScaleFactor);
			QLinearGradient linearGradient(0, 0, 0, o.rect.height() * 0.1);
			linearGradient.setColorAt(0, Qt::white);
			linearGradient.setColorAt(1, QColor(253, 230, 116));
			p.setBrush(linearGradient);
			p.drawPolygon(plus, 13);
		} else {
			if (o.state.testFlag(QStyle::State_MouseOver)) {
				p.setPen(o.palette.highlight().color());
			} else {
				p.setPen(o.palette.mid().color());
			}
			// Frame tab, it is not a rectangle but only 3 lines
			p.drawLine(o.rect.topLeft(), o.rect.bottomLeft());
			p.drawLine(o.rect.topRight(), o.rect.bottomRight());
			p.drawLine(o.rect.topLeft(), o.rect.topRight());
		}
		p.restore();

		// Icon
		QRect r = tabRect(i);
		r.setHeight(fontMetrics().ascent());
		r.translate(3, (height() - r.height()) / 2);
		r.setWidth(r.height() / 2);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		o.icon.paint(&p, r, Qt::AlignLeft | Qt::AlignVCenter);

		// Playlist name
		if (i == currentIndex()) {
			p.setPen(o.palette.windowText().color());
		} else if (o.state.testFlag(QStyle::State_MouseOver)) {
			p.setPen(o.palette.windowText().color());
		} else {
			p.setPen(o.palette.mid().color());
		}
		o.rect.adjust(r.width() + 10, 0, 0, 0);
		p.drawText(o.rect, Qt::AlignLeft | Qt::AlignVCenter, o.text);
	}
}

void TabBar::paintRoundedTabs(QStylePainter &p, QStyleOptionTab &o, int dist)
{
	// A "\+\" button on the right
	static const QPointF plus[13] = {
		QPointF(1, 2), QPointF(2, 2), QPointF(2, 1),
		QPointF(3, 1), QPointF(3, 2), QPointF(4, 2),
		QPointF(4, 3), QPointF(3, 3), QPointF(3, 4),
		QPointF(2, 4), QPointF(2, 3), QPointF(1, 3),
		QPointF(1, 2),
	};

	static const qreal penScaleFactor = 0.15;

	for (int i = 0; i < count(); i++) {
		initStyleOption(&o, i);

		// Background color
		p.save();
		if (i != currentIndex() && i != count() - 1) {
			o.rect.adjust(0, 2, 0, 0);
		} else if (i == count() - 1) {
			o.rect.adjust(2, 2, -4, -4);
		}

		// Highlight the tab under the cursor
		/// TODO

		// Draw last tab frame (which is the [+] button)
		if (i == count() - 1) {
			QPen plusPen;
			if (o.state.testFlag(QStyle::State_MouseOver)) {
				plusPen = QPen(o.palette.highlight(), penScaleFactor);
				p.setPen(o.palette.highlight().color());
				p.setBrush(o.palette.highlight().color().lighter());
			} else {
				plusPen = QPen(o.palette.mid(), penScaleFactor);
				p.setPen(o.palette.mid().color());
				p.setBrush(o.palette.base());
			}
			p.drawRect(o.rect);

			p.translate(o.rect.topLeft());
			plusPen.setJoinStyle(Qt::MiterJoin);
			p.setPen(plusPen);

			// When the tabbar is very big, the inner color of [+] is a gradient like star ratings
			// Should I disable this gradient when height is small?
			p.scale(o.rect.height() * penScaleFactor, o.rect.height() * penScaleFactor);
			QLinearGradient linearGradient(0, 0, 0, o.rect.height() * 0.1);
			linearGradient.setColorAt(0, Qt::white);
			linearGradient.setColorAt(1, QColor(253, 230, 116));
			p.setBrush(linearGradient);
			p.drawPolygon(plus, 13);
		} else {
			if (o.state.testFlag(QStyle::State_MouseOver)) {
				p.setPen(o.palette.highlight().color());
			} else {
				p.setPen(o.palette.mid().color());
			}
			// Rounded frame tab

			QPainterPath pp;
			pp.moveTo(o.rect.x() + dist * 0, o.rect.y() + o.rect.height());
			pp.cubicTo(o.rect.x() + dist * 1, o.rect.y() + o.rect.height(),
					   o.rect.x() + dist * 1, o.rect.y(),
					   o.rect.x() + dist * 2, o.rect.y());
			pp.lineTo(o.rect.x() + o.rect.width() - dist * 1, o.rect.y());
			pp.cubicTo(o.rect.x() + o.rect.width() - dist * 0, o.rect.y(),
					   o.rect.x() + o.rect.width() - dist * 0, o.rect.y() + o.rect.height(),
					   o.rect.x() + o.rect.width() + dist * 1, o.rect.y() + o.rect.height());
			if (i == currentIndex()) {
				p.fillPath(pp, o.palette.base());
			}
			p.setRenderHint(QPainter::Antialiasing, true);
			p.drawPath(pp);
			p.setRenderHint(QPainter::Antialiasing, false);
		}
		p.restore();

		// Icon
		QRect r = tabRect(i);
		r.setHeight(fontMetrics().ascent());
		r.translate(3 + dist, (height() - r.height()) / 2);
		r.setWidth(r.height() / 2);
		p.setRenderHint(QPainter::SmoothPixmapTransform);
		o.icon.paint(&p, r, Qt::AlignLeft | Qt::AlignVCenter);

		// Playlist name
		if (i == currentIndex()) {
			p.setPen(o.palette.windowText().color());
		} else if (o.state.testFlag(QStyle::State_MouseOver)) {
			p.setPen(o.palette.windowText().color());
		} else {
			p.setPen(o.palette.mid().color());
		}
		o.rect.adjust(2 * dist, 0, 0, 0);
		p.drawText(o.rect, Qt::AlignLeft | Qt::AlignVCenter, o.text);
	}
}

/** Rename a tab. */
void TabBar::renameTab()
{
	this->setTabText(currentIndex(), lineEdit->text());
	lineEdit->close();
	for (int t = 0; t < count(); t++) {
		QWidget *button = tabButton(t, QTabBar::RightSide);
		if (button) {
			button->setEnabled(true);
		}
	}
}
