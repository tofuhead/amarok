/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "UserPlaylistModel.h"

#include "Debug.h"
#include "CollectionManager.h"
#include "SqlStorage.h"
#include "SqlPlaylist.h"
#include "TheInstances.h"


#include <KIcon>
#include <QListIterator>


PlaylistBrowserNS::PlaylistModel * PlaylistBrowserNS::PlaylistModel::s_instance = 0;

PlaylistBrowserNS::PlaylistModel * PlaylistBrowserNS::PlaylistModel::instance()
{
    if ( s_instance == 0 )
        s_instance = new PlaylistModel();

    return s_instance;
}


PlaylistBrowserNS::PlaylistModel::PlaylistModel()
 : QAbstractItemModel()
{
    createTables();

    m_root = new SqlPlaylistGroup( "root", 0 );
}


PlaylistBrowserNS::PlaylistModel::~PlaylistModel()
{
}

QVariant
PlaylistBrowserNS::PlaylistModel::data(const QModelIndex & index, int role) const
{
    
    if ( !index.isValid() )
        return QVariant();

    SqlPlaylistViewItem * item = static_cast< SqlPlaylistViewItem* >( index.internalPointer() );
    QString name = item->name();


    if ( role == Qt::DisplayRole || role == Qt::EditRole )
        return name;
    else if (role == Qt::DecorationRole )
        return QVariant( KIcon( "x-media-podcast-amarok" ) );
    else
        return QVariant();

}

QModelIndex
PlaylistBrowserNS::PlaylistModel::index(int row, int column, const QModelIndex & parent) const
{
    DEBUG_BLOCK

    debug() << "row: " << row << ", column: " <<column;
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if ( !parent.isValid() )
    {

        int noOfGroups = m_root->childGroups().count();
        if ( row < noOfGroups ) {
            debug() << "Root playlist group";
            return createIndex( row, column, m_root->childGroups()[row] );
        } else {
            debug() << "Root playlist";
            return createIndex( row, column, m_root->childPlaylists()[row - noOfGroups] );
        }
    }
    else
    {

        SqlPlaylistGroup * playlistGroup = static_cast<SqlPlaylistGroup *>( parent.internalPointer() );
        int noOfGroups = playlistGroup->childGroups().count();

        if ( row < noOfGroups )
            return createIndex( row, column, playlistGroup->childGroups()[row] );
        else
            return createIndex( row, column, playlistGroup->childPlaylists()[row - noOfGroups] );
    }
}

QModelIndex
PlaylistBrowserNS::PlaylistModel::parent( const QModelIndex & index ) const
{
    DEBUG_BLOCK

    if (!index.isValid())
        return QModelIndex();

    SqlPlaylistViewItem * item = static_cast< SqlPlaylistViewItem* >( index.internalPointer() );

    //debug() << "row: " << index.row() << ", name " << item->name() << ", pointer: " << index.internalPointer() << " cast pointer: " << item;

    SqlPlaylistGroup *parent = item->parent();

    debug() << "parent: " << parent;

    if ( parent &&  parent->parent() ){
        int row = parent->parent()->childGroups().indexOf( parent );
        return createIndex( row , 0, parent );
    } else {
        return QModelIndex();
    }



}

int
PlaylistBrowserNS::PlaylistModel::rowCount( const QModelIndex & parent ) const
{
    DEBUG_BLOCK

    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {

        //debug() << "top level item has" << m_root->childCount();

        return m_root->childCount();

    }

    SqlPlaylistViewItem * item = static_cast< SqlPlaylistViewItem* >( parent.internalPointer() );
    debug() << "row: " << parent.row();
    debug() << "address: " << item;
    debug() << "name: " << item->name();

    return item->childCount();
}

int
PlaylistBrowserNS::PlaylistModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

Qt::ItemFlags
PlaylistBrowserNS::PlaylistModel::flags(const QModelIndex & index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
PlaylistBrowserNS::PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch( section )
        {
            case 0: return QString("Name");
            default: return QVariant();
        }
    }

    return QVariant();
}

bool PlaylistBrowserNS::PlaylistModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole)
        return false;
    if ( index.column() != 0 )
        return false;

    SqlPlaylistViewItem * item = static_cast< SqlPlaylistViewItem* >( index.internalPointer() );

    item->rename( value.toString() );

    return true;

}

void PlaylistBrowserNS::PlaylistModel::createTables()
{
    DEBUG_BLOCK;

    SqlStorage *sqlStorage = CollectionManager::instance()->sqlStorage();
    sqlStorage->query( QString( "CREATE TABLE playlist_groups ("
            " id " + sqlStorage->idType() +
            ", parent_id INTEGER"
            ", name " + sqlStorage->textColumnType() +
            ", description " + sqlStorage->textColumnType() + " );" ) );
    sqlStorage->query( "CREATE INDEX parent_podchannel ON playlist_groups( parent_id );" );


    sqlStorage->query( QString( "CREATE TABLE playlists ("
            " id " + sqlStorage->idType() +
            ", parent_id INTEGER"
            ", name " + sqlStorage->textColumnType() +
            ", description " + sqlStorage->textColumnType() + " );" ) );
    sqlStorage->query( "CREATE INDEX parent_playlist ON playlists( parent_id );" );

    sqlStorage->query( QString( "CREATE TABLE playlist_tracks ("
            " id " + sqlStorage->idType() +
            ", playlist_id INTEGER "
            ", track_num INTEGER "
            ", url " + sqlStorage->exactTextColumnType() +
            ", title " + sqlStorage->textColumnType() +
            ", album " + sqlStorage->textColumnType() +
            ", artist " + sqlStorage->textColumnType() +
            ", length INTEGER );" ) );
    sqlStorage->query( "CREATE INDEX parent_playlists_tracks ON playlists_tracks( playlist_id );" );


    sqlStorage->query( "INSERT INTO admin(key,version) "
            "VALUES('AMAROK_PODCAST'," + QString::number( PLAYLIST_DB_VERSION ) + ");" );

}

void PlaylistBrowserNS::PlaylistModel::reloadFromDb()
{
    DEBUG_BLOCK;
    reset();
    m_root->clear();
}

void PlaylistBrowserNS::PlaylistModel::editPlaylist( int id )
{

    //for now, assume that the newly added playlist is in the top level:
    int row = m_root->childGroups().count() - 1;
    foreach ( Meta::SqlPlaylist * playlist, m_root->childPlaylists() ) {
        row++;
        if ( playlist->id() == id ) {
            emit( editIndex( createIndex( row , 0, playlist ) ) );
        }
    }
}






#include "UserPlaylistModel.moc"


