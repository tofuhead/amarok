// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information

#include "k3bexporter.h"
#include "playlist.h"
#include "searchbrowser.h"
#include "threadweaver.h"

#include <qlabel.h>
#include <qpushbutton.h>
#include <qsplitter.h>
#include <qtextstream.h>
#include <qtooltip.h>

#include <kapplication.h> //kapp->config(), QApplication::setOverrideCursor()
#include <kconfig.h>      //config object
#include <kcursor.h>      //waitCursor()
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <kurldrag.h>

#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>


class SearchModule : public ThreadWeaver::Job
{
public:
    static const int ProgressEventType = 8889;
    class ProgressEvent : public QCustomEvent
    {
    public:
        enum State { Start = -1, Stop = -2, Progress = -3 };

        ProgressEvent( int state, KListViewItem* historyItem = 0, KListView* resultView = 0, int count = 0, QString curPath = "", QString curFile = "" )
            : QCustomEvent( ProgressEventType )
            , m_state( state )
            , m_count( count )
            , m_tresultView ( resultView )
            , m_item ( historyItem )
            , m_curPath ( curPath )
            , m_curFile ( curFile ) {}

        int state() { return m_state; }
        int count() { return m_count; }
        KListView* resultView() { return m_tresultView; }
        KListViewItem* item() { return m_item; }
        QString curPath() { return m_curPath; }
        QString curFile() { return m_curFile; }
    private:
        int m_state;
        int m_count;
        KListView* m_tresultView;
        KListViewItem* m_item;
        QString m_curPath;
        QString m_curFile;
    };

    SearchModule( QObject* parent, const QString path, const QString token, KListView* resultView, KListViewItem* historyItem );

    static void stop() { m_stop = true; }
    bool doJob();
    QStringList resultList() { return m_resultList; }
private:

    virtual void completeJob() {}

    void searchDir( QString path );

    static bool m_stop;

    uint resultCount;
    QObject *m_parent;
    QString m_path;
    QString m_token;
    KListView *m_resultView;
    KListViewItem *m_historyItem;
    QStringList m_resultList;
};







SearchBrowser::SearchListView::SearchListView( QWidget *parent, const char *name )
    : KListView ( parent, name )
{}

KURL::List SearchBrowser::SearchListView::selectedUrls()
{
    KURL::List list;

    for( QListViewItemIterator it( this, QListViewItemIterator::Selected); *it; ++it )
        list += KURL( (*it)->text( 2 ) );

    return list;
}

QDragObject *SearchBrowser::SearchListView::dragObject()
{
    return new KURLDrag( selectedUrls(), this );
}


SearchBrowser::HistoryListView::HistoryListView( QWidget *parent, const char *name )
        : KListView ( parent, name )
{}

KURL::List SearchBrowser::HistoryListView::selectedUrls()
{
    KURL::List list;

    setSelected( currentItem(), true );
    QListViewItemIterator it( this, QListViewItemIterator::Selected );
    for(  ; it.current(); ++it )
        list += static_cast<HistoryListView::Item*>(*it)->urlList();

    return list;
}

QDragObject *SearchBrowser::HistoryListView::dragObject()
{
    return new KURLDrag( selectedUrls(), viewport() );
}



SearchBrowser::SearchBrowser( const char *name )
    : QVBox( 0, name )
{
    setSpacing( 4 );
    setMargin( 5 );

    KConfig *config = kapp->config();
    config->setGroup( "SearchBrowser" );

    QHBox *hb1 = new QHBox( this );
    hb1->setSpacing( 4 );
    QLabel *label1 = new QLabel( i18n( "Search &for:" ), hb1 );
    searchEdit = new KLineEdit( hb1 );
    QToolTip::add( searchEdit, i18n( "Enter filename pattern to find." ) );
    label1->setBuddy( searchEdit );

    QHBox *hb2 = new QHBox( this );
    hb2->setSpacing( 4 );
    QLabel *label2 = new QLabel( i18n( "where", "&In:" ), hb2 );
    urlEdit = new KURLComboBox( KURLComboBox::Directories, TRUE, hb2 );
    QToolTip::add( urlEdit, i18n( "Enter folder." ) );
    label2->setBuddy( urlEdit );
    m_searchButton = new QPushButton( i18n( "&Search" ), hb2 );
    urlEdit->setDuplicatesEnabled( false );
    urlEdit->setCompletionObject( new KURLCompletion() );
    urlEdit->setURLs( config->readPathListEntry( "History" ) );
    urlEdit->lineEdit()->setText( config->readPathEntry( "Location", "/" ) );

    splitter    = new QSplitter( Vertical, this );
    resultView  = new SearchListView( splitter );
    historyView = new HistoryListView( splitter );
    QString str = config->readEntry( "Splitter Stream" );
    QTextStream stream( &str, IO_ReadOnly );
    stream >> *splitter; //this sets the splitters position

    resultView->setDragEnabled( TRUE );
    resultView->addColumn( i18n( "File Name" ) );
    resultView->addColumn( i18n( "Directory" ) );
    resultView->setResizeMode( QListView::AllColumns );
    resultView->setSelectionMode( QListView::Extended );
    resultView->setAllColumnsShowFocus( true );

    historyView->setDragEnabled( TRUE );
    historyView->addColumn( i18n( "Search" ) );
    historyView->addColumn( i18n( "Results" ) );
    historyView->addColumn( i18n( "Progress" ) );
    historyView->addColumn( i18n( "Base Folder" ) );
    historyView->setResizeMode( QListView::AllColumns );
    historyView->setSelectionMode( QListView::Extended );
    historyView->setSorting( -1 );
    historyView->setAllColumnsShowFocus( true );

    connect( searchEdit,     SIGNAL( returnPressed() ),    SLOT( slotStartSearch() ) );
    connect( urlEdit,        SIGNAL( returnPressed() ),    SLOT( slotStartSearch() ) );
    connect( m_searchButton, SIGNAL( clicked() ),          SLOT( slotStartSearch() ) );
    connect( resultView,     SIGNAL( doubleClicked( QListViewItem *, const QPoint &, int ) ),
             SLOT( slotDoubleClicked( QListViewItem *, const QPoint &, int ) ) );
    connect( resultView, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
             SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
    connect( historyView, SIGNAL( rightButtonPressed( QListViewItem *, const QPoint &, int ) ),
             SLOT( showContextMenu( QListViewItem *, const QPoint &, int ) ) );
    connect( historyView,    SIGNAL( selectionChanged() ), SLOT( historySelectionChanged() ) );

    setFocusProxy( searchEdit ); //so focus is given to a sensible widget when the tab is opened
}


SearchBrowser::~SearchBrowser()
{
    KConfig *config = kapp->config();
    config->setGroup( "SearchBrowser" );
    config->writePathEntry( "Location", urlEdit->lineEdit()->text() );
    config->writePathEntry( "History", urlEdit->urls() );

    QString str; QTextStream stream( &str, IO_WriteOnly );
    stream << *splitter;

    config->writeEntry( "Splitter Stream", str );
}


void SearchBrowser::slotStartSearch()
{
    QString path = urlEdit->currentText();

    if ( !path.isEmpty() ) //isEmpty() is guarenteed O(1), length() can be O(n)
    {
        urlEdit->insertItem( path );

        // Verify the slash and let KURL parse it
        if ( !path.endsWith( "/" ) )
            path += "/";

        kdDebug() << path << endl;
        KURL *url;
        url = new KURL( path );
        path = url->directory( FALSE, FALSE );
        delete url;

        kdDebug() << path << endl;
        if ( !searchEdit->text().isEmpty() )
        {
            // Create a new item for the HistoryView and pass it to the searching thread
            HistoryListView::Item *historyItem = new HistoryListView::Item( historyView, searchEdit->text() );
            historyItem->setText( 1, "0" );
            historyItem->setText( 2, i18n( "Waiting for other thread" ) );
            historyItem->setText( 3, path );
            historyView->setCurrentItem( historyItem );
            historyView->selectAll( false );
            historyView->setSelected( historyItem, true );

            // Switch button for cancelling
            m_searchButton->setText( i18n( "&Abort" ) );
            disconnect( m_searchButton, SIGNAL( clicked() ), this, SLOT( slotStartSearch() ) );
            connect( m_searchButton, SIGNAL( clicked() ), this, SLOT( stopSearch() ) );

            ThreadWeaver::instance()->queueJob( new SearchModule( this, path, searchEdit->text(), resultView, historyItem ) );
        }
    }
}


void SearchBrowser::showResults( KURL::List list )
{
    resultView->clear();

    const KURL::List::ConstIterator end = list.end();
    for ( KURL::List::ConstIterator it = list.begin(); it != end; ++it ) {
        QString path = (*it).path();
        QString fileName = path.right( path.length() - path.findRev( '/' ) - 1 );
        QString dirPath = path.left( path.findRev( '/' )+1 );

        KListViewItem *resItem = new KListViewItem( resultView, fileName );
        resItem->setText( 1, dirPath );
        resItem->setText( 2, path );
    }
}


void SearchBrowser::customEvent( QCustomEvent *e )
{
    if ( e->type() == (QEvent::Type) SearchModule::ProgressEventType )
    {
        SearchModule::ProgressEvent* p =
            static_cast<SearchModule::ProgressEvent*>( e );

        switch ( p->state() ) {
            case SearchModule::ProgressEvent::Start:
                p->item()->setText( 2, i18n( "Started" ) );
                QApplication::setOverrideCursor( KCursor::workingCursor() );
                resultView->clear();
                break;

            case SearchModule::ProgressEvent::Stop:
                p->item()->setText( 2, i18n( "Done" ) );
                QApplication::restoreOverrideCursor();
                m_searchButton->setText( i18n( "&Search" ) );
                disconnect( m_searchButton, SIGNAL( clicked() ), this, SLOT( stopSearch() ) );
                connect( m_searchButton, SIGNAL( clicked() ), this, SLOT( slotStartSearch() ) );
                break;

            case SearchModule::ProgressEvent::Progress:
                // kdDebug() << "********************************\n";
                // kdDebug() << "SearchModuleEvent arrived, found item: " << p->curPath() << p->curFile() << "\n";
                // kdDebug() << "********************************\n";

                QString curToken = p->curPath();
                if ( curToken.startsWith( p->item()->text( 3 ) ) )
                    curToken = curToken.right( curToken.length() - p->item()->text( 3 ).length() );

                p->item()->setText( 1, QString::number( p->count() ) );
                p->item()->setText( 2, curToken );
                static_cast<HistoryListView::Item*>(p->item())->addUrl( KURL( p->curPath()+p->curFile() ) );

                if( p->item()->isSelected() ) { //add this result to the listview only if it is the current history item
                    KListViewItem *resItem = new KListViewItem( p->resultView(), p->curFile() );
                    resItem->setText( 1, p->curPath() );
                    resItem->setText( 2, p->curPath() + p->curFile() );
                }
        }
    }
}


void SearchBrowser::stopSearch()
{
    // Signal SearchModule job to abort instantly
    SearchModule::stop();
}


void SearchBrowser::slotDoubleClicked( QListViewItem *item, const QPoint &, int )
{
    Playlist::instance()->insertMedia( KURL(item->text( 2 )), Playlist::DirectPlay );
}


void SearchBrowser::showContextMenu( QListViewItem *item, const QPoint &p, int )
{
    if( !item ) return;

    enum Actions { MAKE, APPEND, QUEUE, BURN_DATACD, BURN_AUDIOCD, REMOVE };

    KPopupMenu menu( this );
    menu.insertItem( i18n( "&Append to Playlist" ), APPEND );
    menu.insertItem( i18n( "&Make Playlist" ), MAKE );
    menu.insertItem( i18n( "&Queue After Current Track" ), QUEUE );
    menu.insertSeparator();
    menu.insertItem( i18n("Burn to CD as Data"), BURN_DATACD );
    menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
    menu.insertItem( i18n("Burn to CD as Audio"), BURN_AUDIOCD );
    menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
    if( item->listView() == historyView ) {
        menu.insertSeparator();
        menu.insertItem( i18n( "&Remove" ), REMOVE );
    }

    const KURL::List urls = item->listView() == resultView ? resultView->selectedUrls() : historyView->selectedUrls();

    switch( menu.exec( p ) ) {
        case MAKE:
            Playlist::instance()->clear(); //FALL THROUGH

        case APPEND:
            Playlist::instance()->insertMedia( urls );
            break;

        case QUEUE:
            Playlist::instance()->insertMedia( urls, Playlist::Queue );
            break;

        case BURN_DATACD:
                K3bExporter::instance()->exportTracks( urls, K3bExporter::DataCD );
                break;

        case BURN_AUDIOCD:
                K3bExporter::instance()->exportTracks( urls, K3bExporter::AudioCD );
                break;

        case REMOVE:
            delete item;
            historySelectionChanged();
            break;
    }
}


void SearchBrowser::historySelectionChanged()
{
    showResults( historyView->selectedUrls() );
}




///SearchModule

bool SearchModule::m_stop;

SearchModule::SearchModule( QObject* parent, QString path, QString token, KListView* resultView, KListViewItem* historyItem )
        : ThreadWeaver::Job( "SearchModule" )
        , resultCount( 0 )
        , m_parent( parent )
        , m_path( path )
        , m_token( token )
        , m_resultView( resultView )
        , m_historyItem( historyItem )
{}

bool SearchModule::doJob()
{
    m_stop = false;

    QApplication::postEvent( m_parent, new ProgressEvent( ProgressEvent::Start, m_historyItem ) );
    searchDir( m_path );
    QApplication::postEvent( m_parent, new ProgressEvent( ProgressEvent::Stop, m_historyItem ) );

    return TRUE;
}

void SearchModule::searchDir( QString path )
{
    DIR * d = opendir( QFile::encodeName( path ) );
    if ( d ) {
        dirent * ent;
        while ( ( ent = readdir( d ) ) ) {
            // Check if we shall abort
            if ( m_stop ) return;

            QString file( ent->d_name );

            if ( file != "." && file != ".." ) {
                DIR * t = opendir( QFile::encodeName( path ) + QFile::encodeName( file ) + "/" );
                if ( t ) {
                    closedir( t );
                    searchDir( path + file + "/" );
                } else
                    if ( file.contains( m_token, FALSE ) ) {
                        QApplication::postEvent( m_parent, new ProgressEvent( ProgressEvent::Progress, m_historyItem, m_resultView, ++resultCount, path, file ) );
                        m_resultList.append( path + file );
                    }
            }
        }
        closedir( d );
        free( ent );
    }
}

#include "searchbrowser.moc"
