/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          10/12/1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimain.h"

#include "oddirs.h"
#include "genc.h"
#include "errh.h"

#include "debugmasks.h"

#include <QApplication>
#include <QIcon>

mUseQtnamespace;

#ifdef __mac__
# include "odlogo128x128.xpm"
  const char** uiMain::XpmIconData = od_logo_128x128;
#else
# include "uimainicon.xpm"
  const char** uiMain::XpmIconData = uimainicon_xpm_data;
#endif


void uiMain::setXpmIconData( const char** xpmdata )
{ 
    XpmIconData = xpmdata;
}

#ifdef __win__
# include <QWindowsVistaStyle>
#endif
#ifdef __mac__
# include <QMacStyle>
#endif

#ifdef __lux__
# include <QCleanlooksStyle>
#endif

void myMessageOutput( QtMsgType type, const char *msg );



uiMain* uiMain::themain_ = 0;

uiMain::uiMain( int& argc, char **argv )
    : app_( 0 )
{
    if ( themain_ )
    {
	pErrMsg("You already have a uiMain object!");
        ExitProgram(1);
    }
    
    themain_ = this;
    
    QApplication::setDesktopSettingsAware( true );
    
    QCoreApplication::setOrganizationName( "dGB");
    QCoreApplication::setOrganizationDomain( "opendtect.org" );
    QCoreApplication::setApplicationName( "OpendTect" );
#ifdef __mac__    
    QCoreApplication::setAttribute( Qt::AA_MacDontSwapCtrlAndMeta, true );
#endif
    
#ifndef __win__
    QCoreApplication::addLibraryPath( GetBinPlfDir() );
    // Qt plugin libraries
#endif

    if ( DBG::isOn(DBG_UI) )
	DBG::message( "Constructing QApplication ..." );
    
    app_ = new QApplication( argc, argv );
    
    if ( DBG::isOn(DBG_UI) )
	DBG::message( "... done." );
    
    qInstallMsgHandler( myMessageOutput );

    QStyle* styl = 0;
#ifdef __win__
    if ( !styl )
	styl = QSysInfo)::WindowsVersion == QSysInfo::WV_VISTA
	? new QWindowsVistaStyle
	: new QWindowsXPStyle;
#else
# ifdef __mac__
    if ( !styl )
	styl = new QMacStyle;
# else
    if ( !styl )
	styl = new QCleanlooksStyle;
# endif
#endif
    
    QApplication::setStyle( styl );
    

    app_->setWindowIcon( (QIcon)(XpmIconData) );
}


uiMain::~uiMain()
{
    delete app_;
}


void* uiMain::mainThread()
{ return qApp ? qApp->thread() : 0; }


uiMain& uiMain::theMain()
{ 
    if ( !themain_ )
    {
	pFreeFnErrMsg( "FATAL: no uiMain", "uiMain::theMain()" );
	QApplication::exit( -1 );
    }
    
    return *themain_;
}


void myMessageOutput( QtMsgType type, const char *msg )
{
    switch ( type ) {
	case QtDebugMsg:
	    ErrMsg( msg, true );
	    break;
	case QtWarningMsg:
	    ErrMsg( msg, true );
	    break;
	case QtFatalMsg:
	    ErrMsg( msg );
	    break;
	case QtCriticalMsg:
	    ErrMsg( msg );
	    break;
	default:
	    break;
    }
}


bool isMainThread( const void* thread )
{ return uiMain::theMain().mainThread() == thread; }


bool isMainThreadCurrent()
{ return isMainThread( Threads::currentThread() ); }
