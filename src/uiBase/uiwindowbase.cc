/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiwindowbase.h"

#include <QWidget>


void uiWindowBase::setWindowTitle( const char* txt )
{ getWindow()->setWindowTitle(txt); }

void uiWindowBase::showMinMaxButtons()
{
    mQtclass(Qt)::WindowFlags flags = getWindow()->windowFlags();
    flags |= mQtclass(Qt)::WindowMinMaxButtonsHint;
    getWindow()->setWindowFlags( flags );
}


void uiWindowBase::showAlwaysOnTop()
{
    mQtclass(Qt)::WindowFlags flags = getWindow()->windowFlags();
    flags |= mQtclass(Qt)::WindowStaysOnTopHint;
    getWindow()->setWindowFlags( flags );
}


Threads::Mutex uiWindowBase::windowlistlock_;
ObjectSet<uiWindowBase>	uiWindowBase::windowlist_;


void uiWindowBase::addWindow( uiWindowBase* win )
{
    if ( !win->finalize() )
    {
	pFreeFnErrMsg( "Finalize failed", "uiWindowBase::addWindow" );
	return;
    }
    
    Threads::MutexLocker lock( windowlistlock_ );
    if ( !windowlist_.isPresent( win ) )
	windowlist_ += win;
}


void uiWindowBase::removeWindow( uiWindowBase* win )
{
    Threads::MutexLocker lock( windowlistlock_ );
    windowlist_ -= win;
}


