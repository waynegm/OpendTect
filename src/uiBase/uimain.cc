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
# include <QFusionStyle>
#endif

#if QT_VERSION >= 0x050000
void myMessageOutput( QtMsgType, const QMessageLogContext &, const QString&);
#else
void myMessageOutput( QtMsgType type, const char *msg );
#endif



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
    
#if QT_VERSION >= 0x050000
    qInstallMessageHandler( myMessageOutput );
#else
    qInstallMsgHandler( myMessageOutput );
#endif

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
	styl = new QFusionStyle;
# endif
#endif
    
    QApplication::setStyle( styl );
    
    QPixmap pixmap( XpmIconData );
    app_->setWindowIcon( QIcon(pixmap) );
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


#if QT_VERSION >= 0x050000
void myMessageOutput( QtMsgType type, const QMessageLogContext &,
		      const QString& msg)
#define mMsg msg.toLatin1().constData()
#else
void myMessageOutput( QtMsgType type, const char *msg )
#define mMsg msg
#endif
{
    switch ( type ) {
	case QtDebugMsg:
	    ErrMsg( mMsg, true );
	    break;
	case QtWarningMsg:
	    ErrMsg( mMsg, true );
	    break;
	case QtFatalMsg:
	    ErrMsg( mMsg );
	    break;
	case QtCriticalMsg:
	    ErrMsg( mMsg );
	    break;
	default:
	    break;
    }
}


bool isMainThread( const void* thread )
{ return uiMain::theMain().mainThread() == thread; }


bool isMainThreadCurrent()
{ return isMainThread( Threads::currentThread() ); }
