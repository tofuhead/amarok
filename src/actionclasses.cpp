// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution

#include "actionclasses.h"
#include "amarokconfig.h"
#include "app.h"                //actionCollection() and a SLOT
#include "config.h"             //HAVE_XMMS definition
#include "enginecontroller.h"
#include "playlistwindow.h"     //need amaroK::ToolBar
#include "scriptmanager.h"
#include "socketserver.h"       //Vis::Selector::showInstance(

#include <kaction.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <klocale.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <qtooltip.h>

using namespace amaroK;

KHelpMenu *Menu::s_helpMenu = 0;


static void
safePlug( KActionCollection *ac, const char *name, QWidget *w )
{
    if( ac )
    {
        KAction *a = ac->action( name );
        if( a ) a->plug( w );
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// MenuAction && Menu
// KActionMenu doesn't work very well, so we derived our own
//////////////////////////////////////////////////////////////////////////////////////////

MenuAction::MenuAction( KActionCollection *ac )
  : KAction( i18n( "amaroK Menu" ), 0, ac, "amarok_menu" )
{
    setShortcutConfigurable ( false ); //FIXME disabled as it doesn't work, should use QCursor::pos()
}

int
MenuAction::plug( QWidget *w, int index )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( bar, id );
        connect( bar, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        //TODO create menu on demand
        //TODO create menu above and aligned within window
        //TODO make the arrow point upwards!
        bar->insertButton( QString::null, id, true, i18n( "Menu" ), index );
        bar->alignItemRight( id );

        KToolBarButton* button = bar->getButton( id );
        button->setPopup( amaroK::Menu::instance() );
        button->setName( "toolbutton_amarok_menu" );
        button->setIcon( "configure" );

        return containerCount() - 1;
    }
    else return -1;
}

Menu::Menu()
{
    KActionCollection *ac = pApp->actionCollection();

    setCheckable( true );

    safePlug( ac, "repeat_track", this );
    safePlug( ac, "repeat_playlist", this );
    safePlug( ac, "random_mode", this );

    insertSeparator();

    insertItem( i18n( "&Visualizations..." ), ID_SHOW_VIS_SELECTOR );

    insertSeparator();

    insertItem( i18n( "&Scripts..." ), ID_SHOW_SCRIPT_SELECTOR );
    insertItem( i18n( "&JavaScript Console" ), ID_SHOW_SCRIPT_CONSOLE );

    #ifndef HAVE_KJSEMBED
    setItemEnabled( ID_SHOW_SCRIPT_SELECTOR, false );
    setItemEnabled( ID_SHOW_SCRIPT_CONSOLE, false );
    #endif

    insertSeparator();

    insertItem( i18n( "Configure &Effects..." ), pApp, SLOT( slotConfigEffects() ) );
    insertItem( i18n( "Configure &Decoder..." ), EngineController::engine(), SLOT( configure() ) );

    insertSeparator();

    safePlug( ac, KStdAction::name(KStdAction::ConfigureToolbars), this );
    safePlug( ac, KStdAction::name(KStdAction::KeyBindings), this );
    safePlug( ac, "options_configure_globals", this ); //we created this one
    safePlug( ac, KStdAction::name(KStdAction::Preferences), this );

    insertSeparator();

    insertItem( SmallIcon("help"), i18n( "&Help" ), helpMenu( this ) );

    insertSeparator();

    safePlug( ac, KStdAction::name(KStdAction::Quit), this );

    connect( this, SIGNAL( aboutToShow() ),  SLOT( slotAboutToShow() ) );
    connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );

    #ifndef HAVE_XMMS
    setItemEnabled( ID_SHOW_VIS_SELECTOR, false );
    #endif
}

KPopupMenu*
Menu::instance()
{
    static Menu menu;
    return &menu;
}

KPopupMenu*
Menu::helpMenu( QWidget *parent ) //STATIC
{
    extern KAboutData aboutData;

    if( s_helpMenu == 0 )
        s_helpMenu = new KHelpMenu( parent, &aboutData, pApp->actionCollection() );

    return s_helpMenu->menu();
}

void
Menu::slotAboutToShow()
{
    setItemEnabled( ID_CONF_DECODER, EngineController::engine()->hasConfigure() );
}

void
Menu::slotActivated( int index )
{
    switch( index )
    {
    case ID_SHOW_VIS_SELECTOR:
        Vis::Selector::instance()->show(); //doing it here means we delay creation of the widget
        break;
    #ifdef HAVE_KJSEMBED
    case ID_SHOW_SCRIPT_SELECTOR:
        ScriptManager::Manager::instance()->showSelector();
        break;
    case ID_SHOW_SCRIPT_CONSOLE:
        ScriptManager::Manager::instance()->showConsole();
        break;
    #endif
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PlayPauseAction
//////////////////////////////////////////////////////////////////////////////////////////

PlayPauseAction::PlayPauseAction( KActionCollection *ac )
  : KAction( i18n( "Play/Pause" ), 0, ac, "play_pause" )
{
    EngineController* const ec = EngineController::instance();

    engineStateChanged( EngineController::engine()->state() );

    ec->attach( this );
    connect( this, SIGNAL( activated() ), ec, SLOT( playPause() ) );
}

PlayPauseAction::~PlayPauseAction()
{
    EngineController::instance()->detach( this );
}

void
PlayPauseAction::engineStateChanged( Engine::State state )
{
    switch( state )
    {
    case Engine::Playing:
        setIcon( "player_pause" );
        break;
    default:
        setIcon( "player_play" );
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// AnalyzerAction
//////////////////////////////////////////////////////////////////////////////////////////
#include "blockanalyzer.h"

AnalyzerAction::AnalyzerAction( KActionCollection *ac )
  : KAction( i18n( "Analyzer" ), 0, ac, "toolbar_analyzer" )
{
    setShortcutConfigurable( false );
}

int
AnalyzerAction::plug( QWidget *w, int index )
{
    //NOTE the analyzer will be deleted when the toolbar is deleted or cleared()
    //we are not designed for unplugging() yet so there would be a leak if that happens
    //but it's a rare event and unplugging is complicated.

    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && kapp->authorizeKAction( name() ) )
    {
        const int id = KAction::getToolButtonID();

        addContainer( w, id );
        connect( w, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        QWidget *block = new BlockAnalyzer( w );
        block->setName( "ToolBarAnalyzer" );

        bar->insertWidget( id, 0, block, index );
        bar->setItemAutoSized( id, true );

        return containerCount() - 1;
    }
    else return -1;
}


//////////////////////////////////////////////////////////////////////////////////////////
// VolumeAction
//////////////////////////////////////////////////////////////////////////////////////////

VolumeAction::VolumeAction( KActionCollection *ac )
  : KAction( i18n( "Volume" ), 0, ac, "toolbar_volume" )
  , m_slider( 0 ) //is QGuardedPtr
{
    EngineController::instance()->attach( this );
}

VolumeAction::~VolumeAction()
{
    EngineController::instance()->detach( this );
}

int
VolumeAction::plug( QWidget *w, int index )
{
    //NOTE we only support one plugging currently

    delete (amaroK::Slider*) m_slider; //just in case, remember, we only support one plugging!

    m_slider = new amaroK::Slider( Qt::Vertical, w, amaroK::VOLUME_MAX );
    m_slider->setName( "ToolBarVolume" );
    m_slider->setFixedHeight( 35 ); //FIXME how to determine a sensible height?
    m_slider->setValue( AmarokConfig::masterVolume() );

    QToolTip::add( m_slider, i18n( "Volume Control" ) );

    EngineController* const ec = EngineController::instance();
    connect( m_slider, SIGNAL(sliderMoved( int )), ec, SLOT(setVolume( int )) );
    connect( m_slider, SIGNAL(sliderReleased( int )), ec, SLOT(setVolume( int )) );

    static_cast<KToolBar*>(w)->insertWidget( KAction::getToolButtonID(), 0, m_slider, index );

    return 0;
}

void
VolumeAction::engineVolumeChanged( int value )
{
    if( m_slider ) m_slider->setValue( value );
}



//////////////////////////////////////////////////////////////////////////////////////////
// RandomAction
//////////////////////////////////////////////////////////////////////////////////////////
RandomAction::RandomAction( KActionCollection *ac ) :
    ToggleAction( i18n( "Random &Mode" ), &AmarokConfig::setRandomMode, ac, "random_mode" )
{
    KToggleAction::setChecked( AmarokConfig::randomMode() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// RepeatTrackAction
//////////////////////////////////////////////////////////////////////////////////////////
RepeatTrackAction::RepeatTrackAction( KActionCollection *ac ) :
    ToggleAction( i18n( "Repeat &Track" ), &AmarokConfig::setRepeatTrack, ac, "repeat_track" )
{
    KToggleAction::setChecked( AmarokConfig::repeatTrack() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// RepeatPlaylistAction
//////////////////////////////////////////////////////////////////////////////////////////
RepeatPlaylistAction::RepeatPlaylistAction( KActionCollection *ac ) :
    ToggleAction( i18n( "Repeat &Playlist" ), &AmarokConfig::setRepeatPlaylist, ac, "repeat_playlist" )
{
    KToggleAction::setChecked( AmarokConfig::repeatPlaylist() );
}

#include "actionclasses.moc"
