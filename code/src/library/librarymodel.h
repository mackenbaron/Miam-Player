#ifndef LIBRARYMODEL_H
#define LIBRARYMODEL_H

#include <QFileInfo>
#include <QMap>

#include <fileref.h>
#include <id3v2tag.h>
#include <tag.h>
#include <tlist.h>
#include <textidentificationframe.h>
#include <tstring.h>

#include "filehelper.h"



//class LibraryItem;

#include "libraryitemalbum.h"
#include "libraryitemartist.h"
#include "libraryitemletter.h"
#include "libraryitemtrack.h"

#include <QSet>
#include <QStandardItemModel>

class LibraryModel : public QStandardItemModel
{
	Q_OBJECT

public:
	enum InsertPolicy { Artist,
						Album,
						ArtistAlbum,
						Year };

private:
	QHash<QString, LibraryItemArtist*> _artists;
	QHash<QPair<LibraryItemArtist*, QString>, LibraryItemAlbum*> _albums;
	QHash<QString, LibraryItemAlbum*> _albums2;
	QHash<QString, LibraryItemArtist*> _artistsAlbums;
	QHash<int, LibraryItem*> _years;
	//QMap<LibraryItemAlbum*, QIcon> _albumsWithCovers;
	QSet<QString> _letters;

	// A "cover" is not really a cover, it's just a reference to the upper folder where one track was scanned
	// For a track in ~/music/randomArtist/randomAlbum/track01.mp3, ~/music/randomArtist/randomAlbum is stored
	//QMap<QString, LibraryItemAlbum*> _covers;

	InsertPolicy _currentInsertPolicy;

public:

	LibraryModel(QObject *parent = 0);

	/** Removes everything. */
	void clear();

	//void removeNode(const QModelIndex &index);

	LibraryItem *itemFromIndex(const QModelIndex &index) const {
		return static_cast<LibraryItem*>(QStandardItemModel::itemFromIndex(index));
	}

	QHash<QString, LibraryItemAlbum*> albums() const { return _albums2; }

	InsertPolicy currentInsertPolicy() const { return _currentInsertPolicy; }

private:
	/** Recursively reads the input stream to build nodes and append them to its parent. */
	void loadNode(QDataStream &in, LibraryItem *parent);

	/** Recursively writes nodes to the output stream. */
	void writeNode(QDataStream &dataStream, LibraryItem *parent);

	void insertLetter(const QString &letters);
	void insertTrack(const QString &absFilePath, const FileHelper &fileHelper, InsertPolicy policy);

signals:
	/** A flat file on your computer was successfully loaded. */
	void loadedFromFile();

public slots:
	/** Add (a path to) an icon to every album. */
	/// FIXME
	//void addCoverPathToAlbum(const QString &fileName);

	/** If True, draws one cover before an album name. */
	/// FIXME
	//void displayCovers(bool withCovers);

	/** Build a tree from a flat file saved on disk. */
	void loadFromFile();
	void loadFromFile2();

	/** Read a file from the filesystem and adds it into the library. */
	void readFile(const QString &absFilePath);

	/** Save a tree to a flat file on disk. */
	void saveToFile();
};

#endif // LIBRARYMODEL_H
