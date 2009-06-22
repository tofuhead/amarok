/***************************************************************************
 * Ported to Collection Framework: *
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com> 
 * copyright            : (C) 2009 Seb Ruiz <ruiz@kde.org>

 * Original Work: *
 * copyright            : (C) 2005, 2006 by Martin Aumueller <aumuell@reserv.at>
 * copyright            : (C) 2004 by Christian Muehlhaeuser <chris@chris.de>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#ifndef IPODHANDLER_H
#define IPODHANDLER_H

/* CMake check for GDK */
#include <config-gdk.h>

extern "C" {
  #include <gpod/itdb.h>
}

#include "MediaDeviceMeta.h"
#include "MemoryCollection.h"
//#include "IpodMeta.h"
#include "MediaDeviceHandler.h"
#include "../../../statusbar/StatusBar.h"

#include "kjob.h"
#include <KTempDir>
#include <threadweaver/Job.h>

#include <QObject>
#include <QMap>
#include <QMultiMap>
#include <QMutex>

class QString;
class QFile;
class QDateTime;
class QMutex;

class IpodCollection;


    typedef QMultiMap<QString, Meta::TrackPtr> TitleMap;

// NOTE: podcasts NYI
/*
struct PodcastInfo
{
    // per show
    QString url;
    QString description;
//    QDateTime date;
    QString author;
    bool listened;

    // per channel
    QString rss;

    PodcastInfo() { listened = false; }
};
*/
namespace Meta {
/* The libgpod backend for all Ipod calls */
    class IpodHandler : public MediaDeviceHandler
    {
        Q_OBJECT

        public:
           IpodHandler( IpodCollection *mc, const QString& mountPoint );
           virtual ~IpodHandler();

           virtual bool isWritable() const;

           /* Get Methods */
           QString mountPoint() const { return m_mountPoint; }
           QMap<Meta::TrackPtr, QString> tracksFailed() const { return m_tracksFailed; }

           /// Methods that wrap get/set of information using libgpod

           virtual QString libGetTitle() const;
           virtual QString libGetAlbum() const;
           virtual QString libGetArtist() const;
           virtual QString libGetComposer() const;
           virtual QString libGetGenre() const;
           virtual int     libGetYear() const;
           virtual int     libGetLength() const;
           virtual int     libGetTrackNumber() const;
           virtual QString libGetComment() const;
           virtual int     libGetDiscNumber() const;
           virtual int     libGetBitrate() const;
           virtual int     libGetSamplerate() const;
           virtual float   libGetBpm() const;
           virtual int     libGetFileSize() const;
           virtual int     libGetPlayCount() const;
           virtual uint    libGetLastPlayed() const;
           virtual int     libGetRating() const;
           virtual QString libGetType() const;
           virtual QString libGetPlayableUrl() const;

           virtual void    libSetTitle( const QString& title );
           virtual void    libSetAlbum( const QString& album );
           virtual void    libSetArtist( const QString& artist );
           virtual void    libSetComposer( const QString& composer );
           virtual void    libSetGenre( const QString& genre );
           virtual void    libSetYear( const QString& year );
           virtual void    libSetLength( int length );
           virtual void    libSetTrackNumber( int tracknum );
           virtual void    libSetComment( const QString& comment );
           virtual void    libSetDiscNumber( int discnum );
           virtual void    libSetBitrate( int bitrate );
           virtual void    libSetSamplerate( int samplerate );
           virtual void    libSetBpm( float bpm );
           virtual void    libSetFileSize( int filesize );
           virtual void    libSetPlayCount( int playcount );
           virtual void    libSetLastPlayed( uint lastplayed);
           virtual void    libSetRating( int rating );
           virtual void    libSetType( const QString& type );
           virtual void    libSetPlayableUrl();

           /// Create new track

           virtual void libCreateTrack();
           virtual void findPathToCopy( const Meta::TrackPtr &track );
           virtual bool libCopyTrack( const Meta::TrackPtr &track );
           virtual void addTrackInDB();

           virtual void setCopyTrackForParse();

           /// Parse iteration methods

           virtual void prepareToParse();
           virtual bool isEndOfParseList();
           virtual void prepareToParseNextTrack();
           virtual void nextTrackToParse();
           virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track );

           

           virtual QStringList supportedFormats();
           
           //QPixmap getCover( Meta::MediaDeviceTrackPtr track ) const;
           //void setCoverArt( Itdb_Track *ipodtrack, const QString &filename ) const;
           //void setCoverArt( Itdb_Track *ipodtrack, const QPixmap &image ) const;
           
           /**
            * Successfully read Ipod database?
            */
           //bool succeeded() const { return m_success; }

           /* Set Methods */

           void setMountPoint( const QString &mp) { m_mountPoint = mp; }

           /* Methods Provided for Collection */

           // TODO: abstract copy/delete/database methods
#if 0
           virtual void copyTrackListToDevice( const Meta::TrackList tracklist );
           void deleteTrackListFromDevice( const Meta::TrackList &tracks );
           /**
            * Parses Ipod DB and creates a Meta::IpodTrack
            * for each track in the DB
            */
           //virtual void parseTracks();
           void updateTrackInDB( const KUrl &url, const Meta::TrackPtr &track, Itdb_Track *existingIpodTrack );
           #endif
           virtual void writeDatabase();
           

           // NOTE: do not use writeITunesDB,
           // use the threaded writeDatabase
           bool writeITunesDB( bool threaded=false );

        signals:
           void deleteTracksDone();
           void incrementProgress();
           void endProgressOperation( const QObject *owner );

        public slots:
           bool initializeIpod();

        private:
           /* Handler's Main Methods */

           virtual void prepareToCopy();

           /**
            * @param ipodtrack - track being read from
            * @param track - track being written to
            * Extracts track information from ipodtrack
            * and puts it in track
            */
           //void getBasicIpodTrackInfo( Meta::MediaDeviceTrackPtr ipodtrack ) const;

           /* Handler's Collection Methods */

           //void addIpodTrackToCollection( Itdb_Track *ipodtrack );

           /* libgpod DB Methods */
           // TODO: abstract database methods
/*
           void addTrackInDB( Itdb_Track *ipodtrack );
           void insertTrackIntoDB( const KUrl &url, const Meta::TrackPtr &track );
           bool removeDBTrack( Itdb_Track *track );
*/
           /* libgpod Information Extraction Methods */

           void detectModel();
           KUrl determineURLOnDevice( const Meta::TrackPtr &track );
           QString itunesDir( const QString &path = QString() ) const;
           QString ipodPath( const QString &realPath );
           bool pathExists( const QString &ipodPath, QString *realPath=0 );
           QString realPath( const char *ipodPath );

           /* Cover Art functions */
           //QString ipodArtFilename( const Itdb_Track *ipodtrack ) const;
           //void getCoverArt( const Itdb_Track *ipodtrack );

           /* File I/O Methods */
           // TODO: abstract copy/delete methods (not too bad)
           bool kioCopyTrack( const KUrl &src, const KUrl &dst );
/*
           void deleteNextTrackFromDevice();

           void privateDeleteTrackFromDevice( const Meta::TrackPtr &track );

           void deleteFile( const KUrl &url );
           
*/
           /* Observer Methods */
#if 0
           /** These methods are called when the metadata of a track has changed. They invoke an Ipod DB update */
           virtual void metadataChanged( Meta::TrackPtr track );
           virtual void metadataChanged( Meta::ArtistPtr artist );
           virtual void metadataChanged( Meta::AlbumPtr album );
           virtual void metadataChanged( Meta::GenrePtr genre );
           virtual void metadataChanged( Meta::ComposerPtr composer );
           virtual void metadataChanged( Meta::YearPtr year );
#endif

           /**
            * Handler Variables
            */

           /* Collection Variables */

           // Associated collection
           //IpodCollection   *m_memColl;
           // Map of titles, used to check for duplicate tracks
           TitleMap          m_titlemap;

           /* libgpod variables */
           Itdb_iTunesDB    *m_itdb;
           Itdb_Playlist    *m_masterPlaylist;
           GList            *m_currtracklist;
           Itdb_Track       *m_currtrack;

           /* Lockers */
           QMutex            m_dbLocker; // DB only written by 1 thread at a time
           QMutex            m_joblocker; // lets only 1 job finish at a time
           int               m_jobcounter; // keeps track of copy jobs present

           /* Copy/Delete Variables */
           Meta::TrackList   m_tracksToCopy;
           Meta::TrackList   m_tracksToDelete;

           Itdb_Track       *m_libtrack;
           KUrl              m_copyurl;

           /* Operation Progress Bar */
           ProgressBar      *m_statusbar;

           /* Ipod Connection */
           bool              m_autoConnect;
           QString           m_mountPoint;
           QString           m_name;

           /* Ipod Model */
           bool              m_isShuffle;
           bool              m_isMobile;
           bool              m_isIPhone;

           /* Properties of Ipod */
           bool              m_supportsArtwork;
           bool              m_supportsVideo;
           bool              m_rockboxFirmware;
           bool              m_needsFirewireGuid;

           /* Success/Failure */
           bool m_dbChanged;
           bool m_copyFailed;
           bool m_isCanceled;
           bool m_wait;
           // whether Itdb_Track is created correctly
           bool m_trackCreated;
           // whether read Ipod DB or not
           //bool m_success;

           /* Miscellaneous Variables */

           // Hash that associates an Itdb_Track* to every Track*

           QHash<Meta::MediaDeviceTrackPtr, Itdb_Track*> m_itdbtrackhash;

           // tracks that failed to copy

           QMap<Meta::TrackPtr, QString> m_tracksFailed;

           // tempdir for covers
           KTempDir *m_tempdir;
           QSet<QString> m_coverArt;

           // TODO: Implement lockfile
           // QFile *m_lockFile;

           // TODO: Implement podcasts
           // podcasts
           // Itdb_Playlist* m_podcastPlaylist;

        private slots:
          void fileTransferred( KJob *job );
          //void fileDeleted( KJob *job );

          void slotDBWriteFailed( ThreadWeaver::Job* job );
          void slotDBWriteSucceeded( ThreadWeaver::Job* job );
    };

    class DBWorkerThread : public ThreadWeaver::Job
    {
        Q_OBJECT
        public:
            DBWorkerThread( IpodHandler* handler );
            virtual ~DBWorkerThread();

            virtual bool success() const;

        protected:
            virtual void run();

        private:
            bool m_success;
            IpodHandler *m_handler;
    };
}
#endif
