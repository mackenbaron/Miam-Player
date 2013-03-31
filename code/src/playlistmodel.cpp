#include "playlistmodel.h"

#include "settings.h"

#include <fileref.h>
#include <tag.h>

#include <QTime>
#include <QUrl>

#include <QtDebug>

PlaylistModel::PlaylistModel(QMediaPlaylist *mediaPlaylist) :
	QStandardItemModel(mediaPlaylist), track(-1), qMediaPlaylist(mediaPlaylist)
{}

/** Convert time in seconds into "mm:ss" format. */
QString PlaylistModel::convertTrackLength(int length)
{
	QTime time = QTime(0, 0).addSecs(length);
	// QTime is not designed to handle minutes > 60
	if (time.hour() > 0) {
		return QString::number(time.hour()*60 + time.minute()).append(":").append(time.toString("ss"));
	} else {
		return time.toString("m:ss");
	}
}

/** Clear the content of playlist. */
void PlaylistModel::clear()
{
	// Iterate on the table and always remove the first item
	while (rowCount() > 0) {
		removeRow(0);
	}
	track = -1;
}

void PlaylistModel::move(const QModelIndexList &rows, int destChild)
{
	QModelIndex srcParent = rows.first();
	int srcFirst = rows.first().row();
	int srcLast = rows.last().row();
	QModelIndex parent = rows.first().parent();
	QModelIndex root = invisibleRootItem()->index();
	qDebug() << root << srcFirst << srcLast << destChild;
	// Moving up and down conditions
	if (destChild < srcFirst || destChild > srcLast) {

		bool b = this->beginMoveRows(QModelIndex(), srcFirst, srcLast, QModelIndex(), destChild);
		qDebug() << "success ?" << b;
		if (b) {
			this->endMoveRows();
		}
	}
}

void PlaylistModel::createRows(const QList<QMediaContent> &tracks)
{
	foreach (QMediaContent track, tracks) {
		this->createRow(track);
	}
}

void PlaylistModel::createRow(const QMediaContent &track)
{
	TagLib::FileRef f(track.canonicalUrl().toLocalFile().toStdWString().data());
	if (!f.isNull()) {

		QString title(f.tag()->title().toCString(true));

		// Then, construct a new row with correct informations
		QList<QStandardItem *> widgetItems;
		QStandardItem *trackItem = new QStandardItem(QString::number(f.tag()->track()));
		QStandardItem *titleItem = new QStandardItem(title);
		QStandardItem *albumItem = new QStandardItem(f.tag()->album().toCString(true));
		QStandardItem *lengthItem = new QStandardItem(PlaylistModel::convertTrackLength(f.audioProperties()->length()));
		QStandardItem *artistItem = new QStandardItem(f.tag()->artist().toCString(true));
		QStandardItem *ratingItem = new QStandardItem("***");
		QStandardItem *yearItem = new QStandardItem(QString::number(f.tag()->year()));

		trackItem->setData(track.canonicalUrl());

		widgetItems << trackItem << titleItem << albumItem << lengthItem << artistItem << ratingItem << yearItem;

		int currentRow = rowCount();
		this->insertRow(currentRow);

		QFont font = Settings::getInstance()->font(Settings::PLAYLIST);
		for (int i=0; i < widgetItems.length(); i++) {
			QStandardItem *item = widgetItems.at(i);
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
			item->setFont(font);
			this->setItem(currentRow, i, item);
			/// FIXME ?
			//QFontMetrics fm(font);
			//setRowHeight(currentRow, fm.height());
		}

		trackItem->setTextAlignment(Qt::AlignCenter);
		lengthItem->setTextAlignment(Qt::AlignCenter);
		ratingItem->setTextAlignment(Qt::AlignCenter);
		yearItem->setTextAlignment(Qt::AlignCenter);
	}
}
