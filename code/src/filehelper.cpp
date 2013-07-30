#include "filehelper.h"

#include <QFileInfo>

// Taglib headers
#include <apefile.h>
#include <asffile.h>
#include <flacfile.h>
#include <mpcfile.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <vorbisfile.h>

#include <id3v2tag.h>
#include <id3v2frame.h>

#include <attachedpictureframe.h>
#include <popularimeterframe.h>
#include <tag.h>
#include <tlist.h>
#include <textidentificationframe.h>
#include <tstring.h>

#include <QImage>
#include <QtDebug>

using namespace TagLib;

const QStringList FileHelper::suff = QStringList() << "ape" << "asf" << "flac" << "m4a" << "mpc" << "mp3" << "oga" << "ogg";

FileHelper::FileHelper(FileRef &fileRef, QVariant v)
	: fileType(v.toInt())
{
	f = fileRef.file();
	if (v.toInt() > 0) {
		fileType = v.toInt();
	} else {
		QFileInfo fileInfo(QString(f->name()));
		QString suffix = fileInfo.suffix().toLower();
		if (suffix == "ape") {
			fileType = APE;
		} else if (suffix == "asf") {
			fileType = ASF;
		} else if (suffix == "flac") {
			fileType = FLAC;
		} else if (suffix == "m4a") {
			fileType = MP4;
		} else if (suffix == "mpc") {
			fileType = MPC;
		} else if (suffix == "mp3") {
			fileType = MP3;
		} else if (suffix == "ogg" || suffix == "oga") {
			fileType = OGG;
		}
	}
}

FileHelper::FileHelper(const QString &filePath)
{
	QFileInfo fileInfo(filePath);
	QString suffix = fileInfo.suffix().toLower();
	const char *fp = QFile::encodeName(filePath).constData();
	if (suffix == "ape") {
		f = new APE::File(fp);
		fileType = APE;
	} else if (suffix == "asf") {
		f = new ASF::File(fp);
		fileType = ASF;
	} else if (suffix == "flac") {
		f = new FLAC::File(fp);
		fileType = FLAC;
	} else if (suffix == "m4a") {
		f = new MP4::File(fp);
		fileType = MP4;
	} else if (suffix == "mpc") {
		f = new MPC::File(fp);
		fileType = MPC;
	} else if (suffix == "mp3") {
		f = new MPEG::File(fp);
		fileType = MP3;
	} else if (suffix == "ogg" || suffix == "oga") {
		f = new Vorbis::File(fp);
		fileType = OGG;
	} else {
		f = NULL;
		fileType = -1;
	}
}

QString FileHelper::artistAlbum() const
{
	String artAlb = "";
	APE::File *apeFile = NULL;
	ASF::File *asfFile = NULL;
	FLAC::File *flacFile = NULL;
	MPC::File *mpcFile = NULL;
	MP4::File *mp4File = NULL;
	MPEG::File *mpegFile = NULL;
	Ogg::File *oggFile = NULL;

	switch (fileType) {
	case APE:
		apeFile = static_cast<APE::File*>(f);
		//artAlb = "not yet for ape";
		break;
	case ASF:
		asfFile = static_cast<ASF::File*>(f);
		//artAlb = "not yet for asf";
		break;
	case FLAC:
		flacFile = static_cast<FLAC::File*>(f);
		//artAlb = "not yet for flac";
		break;
	case 3:
		break;
	case MP4:
		mp4File = static_cast<MP4::File*>(f);
		//artAlb = "not yet for mp4";
		break;
	case MPC:
		mpcFile = static_cast<MPC::File*>(f);
		//artAlb = "not yet for mpc";
		break;
	case MP3:
		// For albums with multiple Artists, like OST, the "TPE2" value is commonly used for the tag "Album Artist"
		// It is used in Windows 7, foobar2000, etc
		mpegFile = static_cast<MPEG::File*>(f);
		if (mpegFile->ID3v2Tag()) {
			ID3v2::Tag *tag = mpegFile->ID3v2Tag();
			if (tag) {
				ID3v2::FrameList l = tag->frameListMap()["TPE2"];
				if (!l.isEmpty()) {
					artAlb = l.front()->toString();
				}
			}
		} else if (mpegFile->ID3v1Tag()) {

		}
		break;
	case OGG:
		oggFile = static_cast<Ogg::File*>(f);
		break;
	case 8:
		break;
	case 9:
		break;
	case 10:
		break;
	}
	return QString(artAlb.toCString(true));
}

Cover* FileHelper::extractCover()
{
	MPEG::File *mpegFile = NULL;
	Cover *cover = NULL;
	switch (fileType) {
	case MP3:
		mpegFile = static_cast<MPEG::File*>(f);
		if (mpegFile->ID3v2Tag()) {
			// Look for picture frames only
			ID3v2::FrameList listOfMp3Frames = mpegFile->ID3v2Tag()->frameListMap()["APIC"];
			// It's possible to have more than one picture per file!
			if (!listOfMp3Frames.isEmpty()) {
				for (ID3v2::FrameList::ConstIterator it = listOfMp3Frames.begin(); it != listOfMp3Frames.end() ; it++) {
					// Cast a Frame* to AttachedPictureFrame*
					ID3v2::AttachedPictureFrame *pictureFrame = static_cast<ID3v2::AttachedPictureFrame*>(*it);
					// Performs a deep copy of the cover
					QByteArray b = QByteArray(pictureFrame->picture().data(), pictureFrame->picture().size());
					cover = new Cover(b, QString(pictureFrame->mimeType().toCString(true)));
				}
			}
		} else if (mpegFile->ID3v1Tag()) {
			qDebug() << "FileHelper::extractCover: Not implemented for ID3v1Tag";
		}
		break;
	default:
		break;
	}
	return cover;
}

bool FileHelper::insert(QString key, const QVariant &value)
{
	// Standard tags
	String v = value.toString().toStdString();

	/// XXX Create an enumeration somewhere
	if (key == "ALBUM") {
		f->tag()->setAlbum(v);
	} else if (key == "ARTIST") {
		f->tag()->setArtist(v);
	} else if (key == "COMMENT") {
		f->tag()->setComment(v);
	} else if (key == "GENRE") {
		f->tag()->setGenre(v);
	} else if (key == "TITLE") {
		f->tag()->setTitle(v);
	} else if (key == "TRACKNUMBER") {
		f->tag()->setTrack(value.toInt());
	} else if (key == "YEAR") {
		f->tag()->setYear(value.toInt());
	} else {
		// Other non generic tags, like Artist Album
		APE::File *apeFile = NULL;
		ASF::File *asfFile = NULL;
		FLAC::File *flacFile = NULL;
		MP4::File *mp4File = NULL;
		MPC::File *mpcFile = NULL;
		MPEG::File *mpegFile = NULL;

		switch (fileType) {
		case APE:
			apeFile = static_cast<APE::File*>(f);
			qDebug() << "APE file";
			break;
		case ASF:
			asfFile = static_cast<ASF::File*>(f);
			qDebug() << "ASF file";
			break;
		case FLAC:
			flacFile = static_cast<FLAC::File*>(f);
			qDebug() << "FLAC file";
			break;
		case 3:
			qDebug() << "Mod file";
			break;
		case MP4:
			mp4File = static_cast<MP4::File*>(f);
			qDebug() << "MP4 file";
			break;
		case MPC:
			mpcFile = static_cast<MPC::File*>(f);
			qDebug() << "MPC file";
			break;
		case MP3:
			mpegFile = static_cast<MPEG::File*>(f);
			if (mpegFile->ID3v2Tag()) {
				ID3v2::Tag *tag = mpegFile->ID3v2Tag();
				if (tag) {
					QString convertedKey = this->convertKeyToID3v2Key(key);
					ID3v2::FrameList l = tag->frameListMap()[convertedKey.toStdString().data()];
					if (!l.isEmpty()) {
						tag->removeFrame(l.front());
					}
					ID3v2::TextIdentificationFrame *tif = new ID3v2::TextIdentificationFrame(ByteVector(convertedKey.toStdString().data()));
					tif->setText(value.toString().toStdString().data());
					tag->addFrame(tif);
				}
			} else if (mpegFile->ID3v1Tag()) {
				qDebug() << "ID3v1Tag";
			}
			break;
		case OGG:
			qDebug() << "OGG file";
			break;
		case 8:
			qDebug() << "RIFF file";
			break;
		case 9:
			qDebug() << "TrueAudio file";
			break;
		case 10:
			qDebug() << "WavPack file";
			break;
		}
	}
	return true;
}

int FileHelper::rating() const
{
	MPEG::File *mpegFile = NULL;
	int r = -1;

	/// TODO other types?
	switch (fileType) {
	case MP3:
		mpegFile = static_cast<MPEG::File*>(f);
		if (mpegFile->ID3v2Tag()) {
			ID3v2::FrameList l = mpegFile->ID3v2Tag()->frameListMap()["POPM"];
			if (!l.isEmpty()) {
				ID3v2::PopularimeterFrame *pf = static_cast<ID3v2::PopularimeterFrame*>(l.front());
				if (pf) {
					switch (pf->rating()) {
					case 1:
						r = 1;
						break;
					case 64:
						r = 2;
						break;
					case 128:
						r = 3;
						break;
					case 196:
						r = 4;
						break;
					case 255:
						r = 5;
						break;
					}
				}
			}
		} else if (mpegFile->ID3v1Tag()) {
			qDebug() << "FileHelper::rating: Not implemented for ID3v1Tag";
		}
		break;
	default:
		break;
	}
	return r;
}

void FileHelper::setCover(Cover *cover)
{
	qDebug() << "FileHelper::setCover, cover==NULL?" << (cover == NULL);
	MPEG::File *mpegFile = NULL;
	switch (fileType) {
	case MP3:
		mpegFile = static_cast<MPEG::File*>(f);
		if (mpegFile->ID3v2Tag()) {
			// Look for picture frames only
			ID3v2::FrameList mp3Frames = mpegFile->ID3v2Tag()->frameListMap()["APIC"];
			if (!mp3Frames.isEmpty()) {
				for (ID3v2::FrameList::Iterator it = mp3Frames.begin(); it != mp3Frames.end() ; it++) {
					// Removing a frame will invalidate any pointers on the list
					mpegFile->ID3v2Tag()->removeFrame(*it);
					qDebug() << "removing a frame";
					break;
				}
			}
			if (cover != NULL) {
				ByteVector bv(cover->byteArray().data(), cover->byteArray().length());
				qDebug() << "cover->hasChanged()" << cover->hasChanged();
				ID3v2::AttachedPictureFrame *pictureFrame = new ID3v2::AttachedPictureFrame();
				qDebug() << "cover.mimeType()" << QString(cover->mimeType());
				pictureFrame->setMimeType(cover->mimeType());
				pictureFrame->setPicture(bv);
				mpegFile->ID3v2Tag()->addFrame(pictureFrame);
				qDebug() << "adding a frame";
			}
		} else if (mpegFile->ID3v1Tag()) {
			qDebug() << "FileHelper::setCover: Not implemented for ID3v1Tag";
		}
		break;
	default:
		break;
	}
}

QString FileHelper::convertKeyToID3v2Key(QString key)
{
	/// TODO other relevant keys
	if (key.compare("ARTISTALBUM") == 0) {
		return "TPE2";
	} else {
		return "";
	}
}
