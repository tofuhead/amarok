/***************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org>         *
****************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppletToolbar.h"

#include "AppletToolbarItem.h"
#include "Debug.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsLinearLayout>
#include <QSizePolicy>

Context::AppletToolbar::AppletToolbar( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_appletLayout( 0 )
{    
    m_appletLayout = new QGraphicsLinearLayout( Qt::Horizontal, this );
    
    m_test1 = new AppletToolbarItem( 0 );
    m_test2 = new AppletToolbarItem( 0 );
    
    m_appletLayout->addItem( m_test1 );
    m_appletLayout->addItem( m_test2 );
}

Context::AppletToolbar::~AppletToolbar()
{
    
}

void 
Context::AppletToolbar::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    //DEBUG_BLOCK
    
   // debug() << "drawing rect:" << boundingRect();
    
    // draw translucent curved background
    painter->save();
    QColor fillColor( 176, 176, 176, 225 );
    QPainterPath path;
    path.addRoundedRect( boundingRect(), 5, 5 );
    painter->fillPath( path ,fillColor );
    painter->restore();
    
    
}

void
Context::AppletToolbar::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    debug() << "setting layout to" << QRectF( QPointF( 0, 0 ), event->newSize() );
    m_appletLayout->setGeometry( QRectF( QPointF( 0, 0 ), event->newSize() ) );
}

QSizePolicy 
Context::AppletToolbar::sizePolicy () const
{
    return QSizePolicy( QSizePolicy::Expanding,  QSizePolicy::Fixed );
}



QSizeF
Context::AppletToolbar::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    return QSizeF( constraint.width(), 40 );
}

#include "AppletToolbar.moc"
