/***************************************************************************
                       playlistwidget.cpp  -  description
                          -------------------
 begin                : Don Dez 5 2002
 copyright            : (C) 2002 by Mark Kretschmann
 email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "playlistwidget.h"
#include "playerapp.h"
#include "browserwin.h"
#include "browserwidget.h"
#include "playlistitem.h"

#include "debugareas.h"

#include <qbrush.h>
#include <qcolor.h>
#include <qcursor.h>
#include <qdir.h>
#include <qevent.h>
#include <qfile.h>
#include <qheader.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qrect.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qvaluelist.h>
#include <qwidget.h>

#include <kaccel.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <klineedit.h>
#include <klistview.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <krootpixmap.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kurldrag.h>

#include <kio/netaccess.h>

// CLASS PlaylistWidget --------------------------------------------------------

PlaylistWidget::PlaylistWidget( QWidget *parent, const char *name ) :
        KListView( parent, name ),
        m_rootPixmap( viewport() ),
        m_GlowCount( 100 ),
        m_GlowAdd( 5 ),
        m_undoCounter( 0 )
{
    kdDebug( DA_PLAYLIST ) << "PlaylistWidget::PlaylistWidget()" << endl;

    setName( "PlaylistWidget" );
    setFocusPolicy( QWidget::ClickFocus );
    setShowSortIndicator( true );
    setDropVisualizer( false );      // we handle the drawing for ourselves
    setDropVisualizerWidth( 3 );
    //    setStaticBackground( true );
    //     m_rootPixmap.setFadeEffect( 0.5, Qt::black );
    //     m_rootPixmap.start();

    addColumn( i18n( "Trackname" ), 280 );
    addColumn( i18n( "Title" ), 200 );
    addColumn( i18n( "Artist" ), 100 );
    addColumn( i18n( "Album" ), 100 );
    addColumn( i18n( "Year" ), 40 );
    addColumn( i18n( "Comment" ), 80 );
    addColumn( i18n( "Genre" ), 80 );
    addColumn( i18n( "Directory" ), 80 );

    setSorting( -1 );
    connect( header(), SIGNAL( clicked( int ) ), this, SLOT( slotHeaderClicked( int ) ) );
    connect( this, SIGNAL( contentsMoving( int, int ) ), this, SLOT( slotEraseMarker() ) );

    setCurrentTrack( NULL );
    m_GlowColor.setRgb( 0xff, 0x40, 0x40 );

    m_GlowTimer = new QTimer( this );
    connect( m_GlowTimer, SIGNAL( timeout() ), this, SLOT( slotGlowTimer() ) );
    m_GlowTimer->start( 50 );

    m_pDirLister = new KDirLister();
    m_pDirLister->setAutoUpdate( false );

    initUndo();
}


PlaylistWidget::~PlaylistWidget()
{}


// METHODS -----------------------------------------------------------------

void PlaylistWidget::contentsDragMoveEvent( QDragMoveEvent* e )
{
    e->acceptAction();

    QListViewItem *parent;
    QListViewItem *after;
    findDrop( e->pos(), parent, after );

    QRect tmpRect = drawDropVisualizer( 0, parent, after );

    if ( tmpRect != m_marker )
    {
        slotEraseMarker();
        m_marker = tmpRect;
        viewport() ->repaint( tmpRect );
    }
}


void PlaylistWidget::contentsDragLeaveEvent( QDragLeaveEvent* )
{
    slotEraseMarker();
}


void PlaylistWidget::contentsDropEvent( QDropEvent* e )
{
    slotEraseMarker();

    QListViewItem *parent, *after;
    findDrop( e->pos(), parent, after );

    if ( e->source() == viewport() )
    {
        movableDropEvent( parent, after );
    }
    else
    {
        m_dropRecursionCounter = 0;

        if ( pApp->m_optDropMode == "Recursively" )
            m_dropRecursively = true;
        else
            m_dropRecursively = false;

        KURL::List urlList;

        if ( e->source() == NULL )                       // dragging from outside amarok
        {
            kdDebug( DA_PLAYLIST ) << "dropped item from outside amaroK" << endl;

            if ( !KURLDrag::decode( e, urlList ) || urlList.isEmpty() )
                return ;

            kdDebug( DA_PLAYLIST ) << "dropped item KURL parsed ok" << endl;

            m_pDropCurrentItem = static_cast<PlaylistItem*>( after );
            playlistDrop( urlList );
        }
        else
        {
            PlaylistItem *srcItem, *newItem;
            m_pDropCurrentItem = static_cast<PlaylistItem*>( after );

            srcItem = static_cast<PlaylistItem*>( pApp->m_pBrowserWin->m_pBrowserWidget->firstChild() );
            bool containsDirs = false;

            while ( srcItem != NULL )
            {
                newItem = static_cast<PlaylistItem*>( srcItem->nextSibling() );

                if ( srcItem->isSelected() )
                {
                    urlList.append( srcItem->url() );

                    if ( srcItem->isDir() )
                        containsDirs = true;
                }
                srcItem = newItem;
            }
            if ( containsDirs && pApp->m_optDropMode == "Ask" )
            {
                QPopupMenu popup( this );
                popup.insertItem( i18n( "Add Recursively" ), this, SLOT( slotSetRecursive() ) );
                popup.exec( mapToGlobal( QPoint( e->pos().x() - 120, e->pos().y() - 20 ) ) );
            }

            playlistDrop( urlList );
        }
    }
    writeUndo();
}


void PlaylistWidget::playlistDrop( KURL::List urlList )
{
    ++m_dropRecursionCounter;

    for ( KURL::List::Iterator it = urlList.begin(); it != urlList.end(); it++ )
    {
        //         kdDebug(DA_PLAYLIST) << "dropping item " << (*it).prettyURL() << " to playlist" << endl;

        m_pDirLister->openURL( ( *it ).upURL(), false, false );   // URL; keep = true, reload = true
        while ( !m_pDirLister->isFinished() )
            kapp->processEvents( 300 );

        KFileItem *fileItem = m_pDirLister->findByURL( *it );

        if ( !fileItem )
            kdDebug( DA_PLAYLIST ) << "fileItem is 0!" << endl;

        if ( fileItem && fileItem->isDir() )
        {
            if ( fileItem->isLink() && !pApp->m_optFollowSymlinks && m_dropRecursionCounter >= 2 )
                continue;

            if ( m_dropRecursionCounter >= 50 )        //no infinite loops, please
                continue;

            if ( !m_dropRecursively && m_dropRecursionCounter >= 2 )
                continue;

            m_pDirLister->openURL( *it, false, false );  // URL; keep = false, reload = true
            while ( !m_pDirLister->isFinished() )
                kapp->processEvents( 300 );

            KURL::List dirList;
            AmarokFileList fileList( m_pDirLister->items(), pApp->m_optBrowserSortSpec );
            KFileItemListIterator itSorted( fileList );

            while ( *itSorted )
            {
                if ( ( (*itSorted)->url().path() != "." ) && ( (*itSorted)->url().path() != ".." ) )
                    dirList.append( (*itSorted)->url() );
                ++itSorted;
            }

            playlistDrop( dirList );
        }
        else
        {
            //            kdDebug(DA_PLAYLIST) << "dropping item " << (*it).prettyURL() << " to playlist [2]" << endl;
            if ( pApp->m_pBrowserWin->isFileValid( *it ) )
            {
                m_pDropCurrentItem = addItem( m_pDropCurrentItem, *it );
            }
            else
            {
                if ( m_dropRecursionCounter <= 1 )
                    loadPlaylist( *it, m_pDropCurrentItem );
            }
        }
    }
    --m_dropRecursionCounter;
}


void PlaylistWidget::viewportPaintEvent( QPaintEvent *e )
{
    QListView::viewportPaintEvent( e );

    if ( m_marker.isValid() && e->rect().intersects( m_marker ) )
    {
        QPainter painter( viewport() );
        QBrush brush( QBrush::Dense4Pattern );
        brush.setColor( Qt::red );

        // This is where we actually draw the drop-visualizer
        painter.fillRect( m_marker, brush );
    }
}


QListViewItem* PlaylistWidget::currentTrack()
{
    return m_pCurrentTrack;
}


void PlaylistWidget::setCurrentTrack( QListViewItem *item )
{
    m_pCurrentTrack = item;
}


void PlaylistWidget::unglowItems()
{
    PlaylistItem * item = static_cast<PlaylistItem*>( firstChild() );

    while ( item != NULL )
    {
        if ( item->isGlowing() )
        {
            item->setGlowing( false );
            repaintItem( item );
        }

        item = static_cast<PlaylistItem*>( item->nextSibling() );
    }
}


void PlaylistWidget::triggerSignalPlay()
{
    // FIXME reset play position to start
    pApp->slotPlay();
}


void PlaylistWidget::focusInEvent( QFocusEvent *e )
{
    pApp->m_pBrowserWin->m_pPlaylistLineEdit->setFocus();

    KListView::focusInEvent( e );
}


PlaylistItem* PlaylistWidget::addItem( PlaylistItem *after, KURL url )
{
    PlaylistItem * pNewItem;

    // we're abusing *after as a flag. value 1 == append to list
    if ( ( unsigned long ) after == 1 )
    {
        pNewItem = new PlaylistItem( this, lastItem(), url );
    }
    else
    {
        pNewItem = new PlaylistItem( this, after, url );
    }

    if ( pApp->m_optReadMetaInfo )
    {
        pNewItem->readMetaInfo();
        pNewItem->setMetaTitle();
    }

    return pNewItem;
}


bool PlaylistWidget::loadPlaylist( KURL url, QListViewItem *destination )
{
    bool success = false;
    QString tmpFile;
    PlaylistItem *pCurr = static_cast<PlaylistItem*>( destination );

    if ( url.path().lower().endsWith( ".m3u" ) )
    {
        if ( url.isLocalFile() )
            tmpFile = url.path();
        else
#if KDE_IS_VERSION(3,1,92)
            KIO::NetAccess::download( url, tmpFile, this );
#else
            KIO::NetAccess::download( url, tmpFile );
#endif

        QFile file( tmpFile );
        if ( file.open( IO_ReadOnly ) )
        {
            QString str;
            QTextStream stream( &file );

            while ( ( str = stream.readLine() ) != QString::null )
            {
                if ( !str.startsWith( "#" ) )
                {
                    pCurr = addItem( pCurr, str );
                }
            }
            file.close();
            success = true;
        }
    }
    if ( url.path().lower().endsWith( ".pls" ) )
    {
        if ( url.isLocalFile() )
            tmpFile = url.path();
        else
#if KDE_IS_VERSION(3,1,92)
            KIO::NetAccess::download( url, tmpFile, this );
#else
            KIO::NetAccess::download( url, tmpFile );
#endif

        QFile file( tmpFile );
        if ( file.open( IO_ReadOnly ) )
        {
            QString str;
            QTextStream stream( &file );

            while ( ( str = stream.readLine() ) != QString::null )
            {
                if ( str.startsWith( "File" ) )
                {
                    pCurr = addItem( pCurr, str.section( "=", -1 ) );
                    str = stream.readLine();

                    if ( str.startsWith( "Title" ) )
                        pCurr->setText( 0, str.section( "=", -1 ) );
                }
            }
            file.close();
            success = true;
        }
    }
    KIO::NetAccess::removeTempFile( tmpFile );
    return success;
}


void PlaylistWidget::saveM3u( QString fileName )
{
    QFile file( fileName );

    if ( !file.open( IO_WriteOnly ) )
        return ;

    PlaylistItem* item = static_cast<PlaylistItem*>( firstChild() );
    QTextStream stream( &file );
    stream << "#EXTM3U\n";

    while ( item != NULL )
    {
        if ( item->url().protocol() == "file" )
            stream << item->url().path();
        else
            stream << item->url().url();

        stream << "\n";
        item = static_cast<PlaylistItem*>( item->nextSibling() );
    }

    file.close();
}


// SLOTS ----------------------------------------------

void PlaylistWidget::clear()
{
    KListView::clear();
    setCurrentTrack( NULL );

    emit cleared();
}


void PlaylistWidget::slotGlowTimer()
{
    if ( !isVisible() )
        return ;

    PlaylistItem *item = static_cast<PlaylistItem*>( currentTrack() );

    if ( item != NULL )
    {
        item->setGlowing( true );

        if ( m_GlowCount > 120 )
        {
            m_GlowAdd = -m_GlowAdd;
        }
        if ( m_GlowCount < 90 )
        {
            m_GlowAdd = -m_GlowAdd;
        }
        item->setGlowCol( m_GlowColor.light( m_GlowCount ) );
        repaintItem( item );
        m_GlowCount += m_GlowAdd;
    }
}


void PlaylistWidget::slotSetRecursive()
{
    m_dropRecursively = true;
    kdDebug( DA_PLAYLIST ) << "slotSetRecursive()" << endl;
}


void PlaylistWidget::slotTextChanged( const QString &str )
{
    QListViewItem * pVisibleItem = NULL;
    QListViewItemIterator it( lastItem() );

    while ( *it )
    {
        if ( ( *it )->text( 0 ).lower().contains( str.lower() ) )
        {
            ( *it )->setVisible( true );
            pVisibleItem = ( *it );
        }
        else
            ( *it )->setVisible( false );

        --it;
    }

    clearSelection();
    triggerUpdate();

    if ( pVisibleItem )
    {
        setCurrentItem( pVisibleItem );
        setSelected( pVisibleItem, true );
    }
}


void PlaylistWidget::slotHeaderClicked( int section )
{
    KPopupMenu popup( this );

    popup.insertTitle( i18n( "Sort by " ) + header()->label( section ) );
    int MENU_ASCENDING = popup.insertItem( i18n( "Ascending" ) );
    int MENU_DESCENDING = popup.insertItem( i18n( "Descending" ) );

    QPoint menuPos = QCursor::pos();
    menuPos.setX( menuPos.x() - 20 );
    menuPos.setY( menuPos.y() + 10 );

    int result = popup.exec( menuPos );

    if ( result == MENU_ASCENDING )
    {
        setSorting( section, true );
        sort();
        setSorting( -1 );
    }
    if ( result == MENU_DESCENDING )
    {
        setSorting( section, false );
        sort();
        setSorting( -1 );
    }
}


void PlaylistWidget::slotEraseMarker()
{
    if ( m_marker.isValid() )
    {
        QRect rect = m_marker;
        m_marker = QRect();
        viewport()->repaint( rect, true );
    }
}


// UNDO ==========================================================

void PlaylistWidget::initUndo()
{
    // create undo buffer directory
    m_undoDir.setPath( kapp->dirs()->saveLocation( "data", kapp->instanceName() + "/" ) );

    if ( !m_undoDir.exists( "undo", false ) )
        m_undoDir.mkdir( "undo", false );

    m_undoDir.cd( "undo" );

    // clean directory
    QStringList dirList = m_undoDir.entryList();
    for ( QStringList::Iterator it = dirList.begin(); it != dirList.end(); ++it )
        m_undoDir.remove( *it );
}


void PlaylistWidget::writeUndo()
{
    QString fileName;
    m_undoCounter %= pApp->m_optUndoLevels;
    fileName.setNum( m_undoCounter++ );
    fileName.prepend( m_undoDir.absPath() + "/" );
    fileName += ".m3u";

    if ( m_undoList.count() >= pApp->m_optUndoLevels )
    {
        m_undoDir.remove( m_undoList.first() );
        m_undoList.pop_front();
    }

    saveM3u( fileName );
    m_undoList.append( fileName );
    m_redoList.clear();

    emit sigUndoState( true );
    emit sigRedoState( false );
}



bool PlaylistWidget::canUndo()
{
    if ( m_undoList.isEmpty() )
        return false;
    else
        return true;
}


bool PlaylistWidget::canRedo()
{
    if ( m_redoList.isEmpty() )
        return false;
    else
        return true;
}


void PlaylistWidget::doUndo()
{
    if ( canUndo() )
    {
        m_redoList.append( m_undoList.last() );
        m_undoList.pop_back();

        clear();
        loadPlaylist( m_undoList.last(), 0 );
    }

    emit sigUndoState( canUndo() );
    emit sigRedoState( canRedo() );
}


void PlaylistWidget::doRedo()
{
    if ( canRedo() )
    {
        m_undoList.append( m_redoList.last() );
        m_redoList.pop_back();

        clear();
        loadPlaylist( m_undoList.last(), 0 );
    }

    emit sigUndoState( canUndo() );
    emit sigRedoState( canRedo() );
}


#include "playlistwidget.moc"
