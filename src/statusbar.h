/***************************************************************************
 statusbar.h          : amaroK browserwin statusbar
 copyright            : (C) 2004 by Frederik Holljen
 email                : fh@ez.no
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_STATUSBAR_H
#define AMAROK_STATUSBAR_H

#include <kstatusbar.h>
#include <qlabel.h>

#include "engineobserver.h"

class KAction;
class KProgress;
class KToggleAction;
class QCustomEvent;
class QPushButton;
class QTimer;

namespace amaroK {

class Slider;
class ToggleLabel;

class StatusBar : public KStatusBar, public EngineObserver
{
    Q_OBJECT
public:
    StatusBar( QWidget *parent = 0, const char *name = 0 );
    ~StatusBar();

    static StatusBar* instance() { return m_instance; }

public slots:
    /** update total song count */
    void slotItemCountChanged(int newCount);

protected: /* reimpl from engineobserver */
    virtual void engineStateChanged( Engine::State state );
    virtual void engineTrackPositionChanged( long position );
    virtual void engineNewMetaData( const MetaBundle &bundle, bool trackChanged );

private slots:
    void slotPauseTimer();
    void drawTimeDisplay( int position );
    void stopPlaylistLoader();
    
    void engineMessage( const QString &s ) { message( s, 2000 ); } //NOTE leave inlined!

private:
    virtual void customEvent( QCustomEvent* e );

    static StatusBar* m_instance;

    QLabel         *m_pTimeLabel;
    QLabel         *m_pTitle;
    QLabel         *m_pTotal;
    KProgress      *m_pProgress;
    amaroK::Slider *m_pSlider;
    bool            m_sliderPressed;
    QTimer         *m_pPauseTimer;
    QPushButton    *m_stopPlaylist;
};


class ToggleLabel : public QLabel
{
    Q_OBJECT
public:
    ToggleLabel( const QString&, KStatusBar* const, KToggleAction* const );

protected:
    virtual void mouseDoubleClickEvent ( QMouseEvent* );

public slots:
    void setChecked( bool );

private:
    bool     m_state;
    KAction *m_action;
};

} //namespace amaroK


#endif //AMAROK_STATUSBAR_H
