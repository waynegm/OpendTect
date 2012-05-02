/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uisystemtrayicon.cc,v 1.3 2012-05-02 15:12:02 cvskris Exp $";

#include "uisystemtrayicon.h"
#include "i_qsystemtrayicon.h"

#include "pixmap.h"


uiSystemTrayIcon::uiSystemTrayIcon( const ioPixmap& pm )
    : action_(-1)
    , messageClicked(this)
    , clicked(this)
    , rightClicked(this)
    , middleClicked(this)
    , doubleClicked(this)
{
    qsystemtrayicon_ = new QSystemTrayIcon();
    messenger_ = new QSystemTrayIconMessenger( qsystemtrayicon_, this );
    setPixmap( pm );
}


uiSystemTrayIcon::~uiSystemTrayIcon()
{
    delete messenger_;
    delete qsystemtrayicon_;
}


void uiSystemTrayIcon::setPixmap( const ioPixmap& pm )
{
    QIcon qicon; 
    if ( pm.qpixmap() ) qicon = QIcon( *pm.qpixmap() );
    qsystemtrayicon_->setIcon( qicon );
}


void uiSystemTrayIcon::setToolTip( const char* tt )
{ qsystemtrayicon_->setToolTip( tt ); }

void uiSystemTrayIcon::show()
{ qsystemtrayicon_->show(); }

void uiSystemTrayIcon::hide()
{ qsystemtrayicon_->hide(); }
