/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
________________________________________________________________________

-*/

#include "uimainwin.h"
//#include "uidialog.h"

#include "uiclipboard.h"
#include "uidesktopservices.h"
#include "uidockwin.h"
#include "uifont.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uiseparator.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "envvars.h"
#include "filepath.h"
#include "iopar.h"
#include "keyboardevent.h"
#include "msgh.h"
#include "oddirs.h"
#include "odver.h"
#include "perthreadrepos.h"
#include "settings.h"
#include "strmprov.h"
#include "texttranslator.h"
#include "thread.h"
#include "timer.h"

#include <QAbstractButton>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QPrinter>
#include <QSettings>
#include <QStatusBar>
#include <QWidget>

mUseQtnamespace


class ODMainWindow : public QMainWindow
{ mODTextTranslationClass(ODMainWindow);
friend class		uiMainWin;
public:
			ODMainWindow(uiMainWin& handle,uiParent* parnt,
				     const char* nm,bool modal);
    virtual		~ODMainWindow();

    uiMainWin*		uimainwin();

private:

    uiMainWin&		uimw_;
    int			iconsz_;
    QEventLoop		eventloop_;
};



ODMainWindow::ODMainWindow( uiMainWin& uimw, uiParent* p,
			    const char* nm, bool modal )
    : QMainWindow(p ? p->getParentWidget() : 0,Qt::WindowFlags(Qt::Window))
    , uimw_(uimw)
{
    if ( nm && *nm )
	setObjectName( nm );

    iconsz_ = uiObject::iconSize();
    setIconSize( QSize(iconsz_,iconsz_) );

    setWindowModality( modal ? Qt::WindowModal : Qt::NonModal );
    setDockOptions( VerticalTabs | AnimatedDocks );
}


ODMainWindow::~ODMainWindow()
{
}


uiMainWin* ODMainWindow::uimainwin()
{ return &uimw_; }


// uiMainWin
uiMainWin::uiMainWin( uiParent* p, const uiMainWin::Setup& setup )
    : uiParent(setup.caption_.getFullString())
    , qmainwindow_(0)
    , windowClosed(this)
{
    qmainwindow_ = new ODMainWindow( *this, p, setup.caption_.getFullString(),
				     setup.modal_ );
    qmainwindow_->setWindowIconText(
	setup.caption_.isEmpty() ? "OpendTect" : setup.caption_.getQString() );
    qmainwindow_->setAttribute( Qt::WA_DeleteOnClose, setup.deleteonclose_ );

    maingrp_ = new uiLayoutGroup( p, "Main Group" );
}


uiMainWin::~uiMainWin()
{
}


void uiMainWin::show()
{
    maingrp_->finalise();
    qmainwindow_->show();
}


void uiMainWin::close()
{ qmainwindow_->close(); }

void uiMainWin::raise()
{ qmainwindow_->raise(); }

uiStatusBar* uiMainWin::statusBar()
{ return 0; }

uiMenuBar* uiMainWin::menuBar()
{ return 0; }

void uiMainWin::addToolBar( uiToolBar* tb )
{}


uiToolBar* uiMainWin::removeToolBar( uiToolBar* tb )
{ return 0; }


void uiMainWin::addToolBarBreak()
{ qmainwindow_->addToolBarBreak(); }


uiLayoutMgr* uiMainWin::getLayoutMgr()
{ return maingrp_->getLayoutMgr(); }


uiMainWin* uiMainWin::activeWindow()
{
    QWidget* qwidget = qApp->activeWindow();
    if ( !qwidget )
	return 0;

    ODMainWindow* odmw = dynamic_cast<ODMainWindow*>(qwidget);
    if ( !odmw )
	return 0;

    return odmw->uimainwin();
}


uiString uiMainWin::uniqueWinTitle( const uiString& txt,
				    QWidget* forwindow,
				    BufferString* outputaddendum )
{
    const QWidgetList toplevelwigs = qApp->topLevelWidgets();

    uiString res;
    for ( int count=1; true; count++ )
    {
	bool unique = true;
	uiString beginning = txt.isEmpty() ? tr("<no title>") : txt;

	if ( count>1 )
	{
	    res = toUiString( "%1 {%2}").arg( beginning )
		                        .arg( toUiString(count) );
	    BufferString addendum( "  {", toString(count), "}" );
	    if ( outputaddendum ) *outputaddendum = addendum;
	}
	else
	{
	    res = beginning;
	}

	QString wintitle = res.getQString();

	for ( int idx=0; idx<toplevelwigs.count(); idx++ )
	{
	    const QWidget* qw = toplevelwigs.at( idx );
	    if ( !qw->isWindow() || qw->isHidden() || qw==forwindow )
		continue;

	    if ( qw->windowTitle() == wintitle )
	    {
		unique = false;
		break;
	    }
	}

	if ( unique ) break;
    }

    return res;
}




#if 0
static Threads::Mutex		winlistmutex_;
static ObjectSet<uiMainWin>	orderedwinlist_;
static uiMainWin*		programmedactivewin_ = 0;


static void addToOrderedWinList( uiMainWin* uimw )
{
    winlistmutex_.lock();
    orderedwinlist_ -= uimw;
    orderedwinlist_ += uimw;
    winlistmutex_.unLock();
}


static bool isInOrderedWinList( const uiMainWin* uimw )
{
    winlistmutex_.lock();
    const bool res = orderedwinlist_.isPresent( uimw );
    winlistmutex_.unLock();
    return res;
}


//=============================================================================

class ODMainWindow : public QMainWindow
{ mODTextTranslationClass(ODMainWindow);
friend class		uiMainWin;
public:
			ODMainWindow(uiMainWin& handle,uiParent* parnt,
				      const char* nm,bool modal);
    virtual		~ODMainWindow();

    void		construct(int nrstatusflds,bool wantmenubar);

    uiStatusBar*	uistatusbar();
    uiMenuBar*		uimenubar();

    virtual void        polish();
    void		reDraw(bool deep);
    void		go(bool showminimized=false);
    virtual void	show()				{ doShow(); }

    void		move(uiMainWin::PopupArea);
    void		move(int,int);

    void		close();
    bool		touch();

    void		removeDockWin(uiDockWin*);
    void		addDockWin(uiDockWin&,uiMainWin::Dock);

    virtual QMenu*	createPopupMenu();
    void		addToolBar(uiToolBar*);
    uiToolBar*		removeToolBar(uiToolBar*);
    uiMenu&		getToolbarsMenu()		{ return *toolbarsmnu_;}
    void		updateToolbarsMenu();

    const ObjectSet<uiToolBar>& toolBars() const	{ return toolbars_; }
    const ObjectSet<uiDockWin>& dockWins() const	{ return dockwins_; }

    void		setModal(bool yn);
    bool		isModal() const			{ return modal_; }

    void		activateInGUIThread(const CallBack&,bool busywait);

protected:

    virtual void	finalise()	{ finalise(false); }
    virtual void	finalise(bool trigger_finalise_start_stop);
    void		closeEvent(QCloseEvent*);
    bool		event(QEvent*);

    void		keyPressEvent(QKeyEvent*);

    void		doShow(bool minimized=false);
    void		managePopupPos();


    void		renewToolbarsMenu();
    void		toggleToolbar(CallBacker*);

    void		saveSettings();
    void		readSettings();

    bool		exitapponclose_;

    Threads::Mutex	activatemutex_;
    ObjectSet<CallBack>	activatecbs_;
    int			nractivated_;

    int			eventrefnr_;

    uiStatusBar*	statusbar_;
    uiMenuBar*		menubar_;
    uiMenu*		toolbarsmnu_;

    ObjectSet<uiToolBar> toolbars_;
    ObjectSet<uiDockWin> dockwins_;
    uiString		windowtitle_;

private:

    uiMainWin&		uimw_;
    QEventLoop		eventloop_;

    int			iconsz_;
    bool		modal_;
    Qt::WindowFlags	getFlags(bool hasparent,bool modal) const;

    uiSize		prefsz_;
    uiPoint		prefpos_;
    bool		moved_;
    bool		createtbmenu_;

    bool		deletefrombody_;
    bool		deletefromod_;

    bool		hasguisettings_;
};


ODMainWindow::ODMainWindow( uiMainWin& uimw, uiParent* p,
			    const char* nm, bool modal )
	: QMainWindow(p ? p->getParentWidget() : 0,getFlags(p,modal))
	, uimw_(uimw)
	, centralwidget_(0)
	, statusbar_(0)
	, menubar_(0)
	, toolbarsmnu_(0)
	, modal_(p && modal)
	, exitapponclose_(false)
	, prefsz_(-1,-1)
	, prefpos_(uiPoint::udf())
	, nractivated_(0)
	, moved_(false)
	, createtbmenu_(false)
	, hasguisettings_(false)
{
    if ( nm && *nm )
	setObjectName( nm );

    iconsz_ = uiObject::iconSize();
    setIconSize( QSize(iconsz_,iconsz_) );

    setWindowModality( p && modal ? Qt::WindowModal
				  : Qt::NonModal );

    setDockOptions( VerticalTabs | AnimatedDocks );

    deletefrombody_ = deletefromod_ = false;
}


ODMainWindow::~ODMainWindow()
{
    deleteAllChildren(); //delete them now to make sure all ui objects
			 //are deleted before their body counterparts

    deepErase( toolbars_ );

    if ( toolbarsmnu_ )
    {
	toolbarsmnu_->clear();
	if ( toolbarsmnu_->isStandAlone() )
	    delete toolbarsmnu_;
    }

    if ( !deletefromod_ )
    {
	deletefrombody_ = true;
	delete &uimw_;
    }

    delete statusbar_;
    delete menubar_;
}


void ODMainWindow::setModal( bool yn )
{
    modal_ = yn;
    setWindowModality( yn ? Qt::WindowModal
			  : Qt::NonModal );
}


Qt::WindowFlags ODMainWindow::getFlags( bool hasparent, bool modal ) const
{ return Qt::WindowFlags( Qt::Window ); }



void ODMainWindow::doShow( bool minimized )
{
    uimw_.updateCaption();
    eventrefnr_ = uimw_.beginCmdRecEvent("WinPopUp");
    managePopupPos();

    if ( minimized )
	QMainWindow::showMinimized();
    else
    {
	if ( isMinimized() ) showNormal();
	raise();
	QMainWindow::show();
    }

    QEvent* ev = new QEvent( mUsrEvPopUpReady );
    QApplication::postEvent( this, ev );

#ifdef __debug__

/*
 We need a check on windows being too big for little (laptop) screens.
 But if we set the margins too tight we'll get a hit for many windows.
 Then we (programmers) will start ignoring pErrMsg's, which is _really_ bad.
 Notes:
 * It seems that windows can be a bit bigger than the screen.
 * Remember the actual size is dep on font size.
 * I asked Farrukh to come up with some data on our laptops, most notably the
   ones used for the courses, but it's inconclusive.

 The issue is the tension between: what would we like to support vs the ease
 of build and - last but not least - the convenience for the user to have
 a lot of info and tools on a single window.

 In any case: recurring pErrMsg's are *BAD*. They should never be ignored.
 Which means they have to indicate serious problems, not matters of taste.

*/

#   define mMinSupportedWidth 1920
#   define mMinSupportedHeight 1080

    QRect qrect = geometry();
    if ( !hasguisettings_ && (qrect.width() > mMinSupportedWidth
			   || qrect.height() > mMinSupportedHeight) )
    {
	BufferString msg( "The window '", name(), "' is " );
	msg.add( qrect.width() ).add( "x" ).add( qrect.height() )
	    .add( ". That won't fit on many laptops.\nWe want to support >= " )
	    .add( mMinSupportedWidth ).add( "x" ).add( mMinSupportedHeight )
	    .add( ", see comments in the .cc file." );
	pErrMsg( msg );
    }

#endif

    if ( !uimw_.afterPopup.isEmpty() )
    {
	uimw_.afterpopuptimer_ = new Timer( "After popup timer" );
	uimw_.afterpopuptimer_->tick.notify(
				mCB(&uimw_,uiMainWin,aftPopupCB) );
	uimw_.afterpopuptimer_->start( 50, true );
    }

    if ( modal_ )
	eventloop_.exec();
}


void ODMainWindow::construct( int nrstatusflds, bool wantmenubar )
{
    centralwidget_ = new uiGroup( &handle(), "OpendTect Main Window" );
    setCentralWidget( centralwidget_->body()->qwidget() );

    centralwidget_->setIsMain(true);
    centralwidget_->setBorder(10);
    centralwidget_->setStretch(2,2);

    if ( nrstatusflds != 0 )
    {
	QStatusBar* mbar= statusBar();
	if ( mbar )
	    statusbar_ = new uiStatusBar( &handle(),
					  "MainWindow StatusBar handle", *mbar);
	else
	    { pErrMsg("No statusbar returned from Qt"); }

	if ( nrstatusflds > 0 )
	{
	    for( int idx=0; idx<nrstatusflds; idx++ )
		statusbar_->addMsgFld();
	}
    }
    if ( wantmenubar )
    {
	QMenuBar* qmenubar = menuBar();
	if ( qmenubar )
	    menubar_ = new uiMenuBar( &handle(), "MenuBar", qmenubar );
	else
	    { pErrMsg("No menubar returned from Qt"); }

	toolbarsmnu_ = new uiMenu( &handle(), tr("Toolbars") );
    }
}


void ODMainWindow::move( uiMainWin::PopupArea pa )
{
    QDesktopWidget wgt;
    const int xpos = wgt.screen()->width() - QMainWindow::width();
    const int ypos = wgt.screen()->height() - QMainWindow::height();

    switch( pa )
    {
	case uiMainWin::TopLeft :
	    move( 0, 0 ); break;
	case uiMainWin::TopRight :
	    move( xpos, 0 ); break;
	case uiMainWin::BottomLeft :
	    move( 0, ypos ); break;
	case uiMainWin::BottomRight :
	    move( xpos, ypos ); break;
	case uiMainWin::Middle :
	    move( mNINT32(((float) xpos)/2), mNINT32(((float) ypos) / 2));break;
	default:
	    break;
    }
}


void ODMainWindow::move( int xdir, int ydir )
{
    QWidget::move( xdir, ydir );
    moved_ = true;
}


void ODMainWindow::polish()
{ QMainWindow::ensurePolished(); }


void ODMainWindow::reDraw( bool deep )
{
    update();
    centralwidget_->reDraw( deep );
}


void ODMainWindow::go( bool showminimized )
{
    finalise( true );
    doShow( showminimized );
    move( uimw_.popuparea_ );
}


QMenu* ODMainWindow::createPopupMenu()
{ return createtbmenu_ ? QMainWindow::createPopupMenu() : 0; }


void ODMainWindow::finalise( bool trigger_finalise_start_stop )
{
    if ( trigger_finalise_start_stop )
	uimw_.preFinalise().trigger( uimw_ );

    centralwidget_->finalise();
    finaliseChildren();

    if ( trigger_finalise_start_stop )
	uimw_.postFinalise().trigger( uimw_ );
}


void ODMainWindow::closeEvent( QCloseEvent* ce )
{
    const int refnr = uimw_.beginCmdRecEvent( "Close" );

    if ( uimw_.closeOK() )
    {
	uimw_.windowClosed.trigger( uimw_ );
	ce->accept();

	if ( isInOrderedWinList(&uimw_) && modal_ )
	    eventloop_.exit();
    }
    else
	ce->ignore();

     uimw_.endCmdRecEvent( refnr, "Close" );
}


void ODMainWindow::close()
{
    if ( !uimw_.closeOK() ) return;

    uimw_.windowClosed.trigger( uimw_ );

    if ( !isInOrderedWinList(&uimw_) )
	return;

    if ( testAttribute(Qt::WA_DeleteOnClose) )
    {
	QMainWindow::close();
	return;
    }

    if ( modal_ )
	eventloop_.exit();

    QMainWindow::hide();

    if ( exitapponclose_ )
	qApp->quit();
}


uiStatusBar* ODMainWindow::uistatusbar()
{ return statusbar_; }

uiMenuBar* ODMainWindow::uimenubar()
{ return menubar_; }


void ODMainWindow::removeDockWin( uiDockWin* dwin )
{
    if ( !dwin ) return;

    removeDockWidget( dwin->getDockWidget() );
    dockwins_ -= dwin;
}


void ODMainWindow::addDockWin( uiDockWin& dwin, uiMainWin::Dock dock )
{
    Qt::DockWidgetArea dwa = Qt::LeftDockWidgetArea;
    if ( dock == uiMainWin::Right ) dwa = Qt::RightDockWidgetArea;
    else if ( dock == uiMainWin::Top ) dwa = Qt::TopDockWidgetArea;
    else if ( dock == uiMainWin::Bottom ) dwa =
					     Qt::BottomDockWidgetArea;
    addDockWidget( dwa, dwin.getDockWidget() );
    if ( dock == uiMainWin::TornOff )
	dwin.setFloating( true );
    dockwins_ += &dwin;
}


void ODMainWindow::toggleToolbar( CallBacker* cb )
{
    mDynamicCastGet( uiAction*, action, cb );
    if ( !action ) return;

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	uiToolBar& tb = *toolbars_[idx];
	if ( tb.name()==action->text().getFullString() )
	    tb.display( tb.isHidden() );
    }
}


void ODMainWindow::updateToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    const ObjectSet<uiAction>& items = toolbarsmnu_->actions();

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	const uiToolBar& tb = *toolbars_[idx];
	uiAction& action = *const_cast<uiAction*>( items[idx] );
	if ( tb.name()==tb.name() )
	    action.setChecked( !tb.isHidden() );
    }
}


void ODMainWindow::addToolBar( uiToolBar* tb )
{
    if ( toolbars_.isPresent(tb) )
	{ pErrMsg("Toolbar is already added"); return; }
    QMainWindow::addToolBar( (Qt::ToolBarArea)tb->prefArea(),
			      tb->getQToolbar() );
    toolbars_ += tb;
    renewToolbarsMenu();
}


uiToolBar* ODMainWindow::removeToolBar( uiToolBar* tb )
{
    if ( !toolbars_.isPresent(tb) )
	return 0;

    QMainWindow::removeToolBar( tb->getQToolbar() );
    toolbars_ -= tb;
    renewToolbarsMenu();
    return tb;
}


void ODMainWindow::renewToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    for ( int idx=0; idx<toolbars_.size(); idx++ )
	toolbars_[idx]->setToolBarMenuAction( 0 );

    toolbarsmnu_->clear();
    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	uiToolBar& tb = *toolbars_[idx];
	uiAction* itm =
	    new uiAction( toUiString(tb.name()),
	    mCB(this,ODMainWindow,toggleToolbar) );
	toolbarsmnu_->insertItem( itm );
	tb.setToolBarMenuAction( itm );
	itm->setCheckable( true );
    }
}


static BufferString getSettingsFileName()
{
    BufferString fnm( "qtsettings_", (int)mODVersion );
    FilePath fp( GetSettingsDir(), fnm );
    const char* swusr = GetSoftwareUser();
    if ( swusr )
	fp.setExtension( swusr );
    return fp.fullPath();
}


void ODMainWindow::saveSettings()
{
    const BufferString fnm = getSettingsFileName();
    QSettings settings( fnm.buf(), QSettings::IniFormat );
    settings.beginGroup( NamedObject::name().buf() );
    settings.setValue( "size", size() );
    settings.setValue( "pos", pos() );
    settings.setValue( "state", saveState() );
    settings.endGroup();
}


void ODMainWindow::readSettings()
{
    const BufferString fnm = getSettingsFileName();
    QSettings settings( fnm.buf(), QSettings::IniFormat );
    settings.beginGroup( NamedObject::name().buf() );
    QSize qsz( settings.value("size", QSize(200,200)).toSize() );
    prefsz_ = uiSize( qsz.width(), qsz.height() );
    QPoint qpt( settings.value("pos", QPoint(200,200)).toPoint() );
    prefpos_.setXY( qpt.x(), qpt.y() );
    restoreState( settings.value("state").toByteArray() );
    settings.endGroup();

    updateToolbarsMenu();
    hasguisettings_ = true;
}


#define mExecMutex( statements ) \
    activatemutex_.lock(); statements; activatemutex_.unLock();


void ODMainWindow::activateInGUIThread( const CallBack& cb, bool busywait )
{
    CallBack* actcb = new CallBack( cb );
    mExecMutex( activatecbs_ += actcb );

    QEvent* guithreadev = new QEvent( mUsrEvGuiThread );
    QApplication::postEvent( this, guithreadev );

    float sleeptime = 0.01;
    while ( busywait )
    {
	mExecMutex( const int idx = activatecbs_.indexOf(actcb) );
	if ( idx < 0 )
	    break;

	Threads::sleep( sleeptime );
	if ( sleeptime < 1.28 )
	    sleeptime *= 2;
    }
}


void ODMainWindow::keyPressEvent( QKeyEvent* ev )
{
    OD::KeyboardKey key = OD::KeyboardKey( ev->key() );
    OD::ButtonState modifier = OD::ButtonState( (int)ev->modifiers() );

    if ( key == OD::KB_C && modifier == OD::ControlButton )
	uimw_.ctrlCPressed.trigger();

    return QMainWindow::keyPressEvent( ev );
}


bool ODMainWindow::event( QEvent* ev )
{
    if ( ev->type() == mUsrEvGuiThread )
    {
	mExecMutex( CallBack* actcb = activatecbs_[nractivated_++] );
	actcb->doCall( this );
	uimw_.activatedone.trigger( actcb->cbObj() );
	mExecMutex( activatecbs_ -= actcb; nractivated_-- );
	delete actcb;
    }
    else if ( ev->type() == mUsrEvPopUpReady )
    {
	uimw_.endCmdRecEvent( eventrefnr_, "WinPopUp" );
    }
    else
	return QMainWindow::event( ev );

    return true;
}


void ODMainWindow::managePopupPos()
{
    uiParent* myparent = uimw_.parent();
    uiMainWin* myparentsmw = myparent ? myparent->mainwin() : 0;
    if ( myparentsmw && !myparentsmw->isHidden() )
	return;

    uiMainWin* parwin = uimw_.programmedActiveWindow();
    while ( parwin && parwin->isHidden() )
	parwin = parwin->parent() ? parwin->parent()->mainwin() : 0;

    if ( !parwin || moved_ )
	return;

    const uiRect pwrect = parwin->geometry( false );
    uimw_.setCornerPos( pwrect.get(uiRect::Left), pwrect.get(uiRect::Top) );
    moved_ = false;
}


//===== uiMainWin ============================================================


uiMainWin::uiMainWin( uiParent* p, const uiMainWin::Setup& setup )
    : uiParent(setup.caption_.getFullString(),0)
    , qmainwindow_(0)
    , parent_(p)
    , popuparea_(Auto)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , caption_(setup.caption_)
    , afterpopuptimer_(0)
    , languagechangecount_( TrMgr().changeCount() )
{
    qmainwindow_ = new ODMainWindow( *this, p, setup.caption_.getFullString(),
			       setup.modal_ );
    setBody( qmainwindow_ );
    qmainwindow_->construct( setup.nrstatusflds_, setup.withmenubar_ );
    qmainwindow_->setWindowIconText( setup.caption_.isEmpty()
		? "OpendTect"
		: setup.caption_.getQString() );
    qmainwindow_->setAttribute( Qt::WA_DeleteOnClose, setup.deleteonclose_ );
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );
}


uiMainWin::uiMainWin( uiParent* parnt, const uiString& cpt,
		      int nrstatusflds, bool withmenubar, bool modal )
    : uiParent(cpt.getFullString(),0)
    , qmainwindow_(0)
    , parent_(parnt)
    , popuparea_(Auto)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , caption_(cpt)
    , afterpopuptimer_(0)
    , languagechangecount_( TrMgr().changeCount() )
{
    qmainwindow_ = new ODMainWindow( *this, parnt, caption_.getFullString(), modal );
    setBody( qmainwindow_ );
    qmainwindow_->construct( nrstatusflds, withmenubar );
    qmainwindow_->setWindowIconText( caption_.isEmpty()
			     ? "OpendTect"
			     : caption_.getQString() );
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );

    mAttachCB( TrMgr().languageChange, uiMainWin::languageChangeCB );
}


uiMainWin::uiMainWin( uiString nm, uiParent* parnt )
    : uiParent(nm.getFullString(),0)
    , qmainwindow_(0)
    , parent_(parnt)
    , popuparea_(Auto)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , caption_(nm)
    , afterpopuptimer_(0)
{
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );

    mAttachCB( TrMgr().languageChange, uiMainWin::languageChangeCB );
}


uiMainWin::~uiMainWin()
{
    detachAllNotifiers();

    if ( !qmainwindow_->deletefrombody_ )
    {
	qmainwindow_->deletefromod_ = true;
	delete qmainwindow_;
    }

    winlistmutex_.lock();
    orderedwinlist_ -= this;

    if ( programmedactivewin_ == this )
	programmedactivewin_ = parent() ? parent()->mainwin() : 0;

    delete afterpopuptimer_;
    winlistmutex_.unLock();
}


QWidget* uiMainWin::getWidget( int )
{ return qmainwindow_; }


uiStatusBar* uiMainWin::statusBar()		{ return qmainwindow_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return qmainwindow_->uimenubar(); }


void uiMainWin::show()
{
    addToOrderedWinList( this );
    qmainwindow_->go();
}


void uiMainWin::close()				{ qmainwindow_->close(); }
void uiMainWin::reDraw(bool deep)		{ qmainwindow_->reDraw(deep); }
bool uiMainWin::touch()				{ return qmainwindow_->touch(); }
bool uiMainWin::finalised() const		{ return qmainwindow_->finalised(); }
void uiMainWin::setExitAppOnClose( bool yn )	{ qmainwindow_->exitapponclose_ = yn; }
void uiMainWin::showMaximized()			{ qmainwindow_->showMaximized(); }
void uiMainWin::showMinimized()			{ qmainwindow_->showMinimized(); }
void uiMainWin::showNormal()			{ qmainwindow_->showNormal(); }
bool uiMainWin::isMaximized() const		{ return qmainwindow_->isMaximized(); }
bool uiMainWin::isMinimized() const		{ return qmainwindow_->isMinimized(); }
bool uiMainWin::isHidden() const		{ return qmainwindow_->isHidden(); }
bool uiMainWin::isModal() const			{ return qmainwindow_->isModal(); }


void uiMainWin::setCaption( const uiString& txt )
{
    caption_ = txt;
    updateCaption();
}


void uiMainWin::updateCaption()
{
    uniquecaption_ = uniqueWinTitle(caption_,qmainwindow_,0);
    qmainwindow_->setWindowTitle( uniquecaption_.getQString() );
}


const uiString& uiMainWin::caption( bool unique ) const
{
    return unique ? uniquecaption_ : caption_;
}


void uiMainWin::setDeleteOnClose( bool yn )
{ qmainwindow_->setAttribute( Qt::WA_DeleteOnClose, yn ); }


void uiMainWin::removeDockWindow( uiDockWin* dwin )
{ qmainwindow_->removeDockWin( dwin ); }


void uiMainWin::addDockWindow( uiDockWin& dwin, Dock d )
{ qmainwindow_->addDockWin( dwin, d ); }


void uiMainWin::addToolBar( uiToolBar* tb )
{ qmainwindow_->addToolBar( tb ); }


uiToolBar* uiMainWin::removeToolBar( uiToolBar* tb )
{ return qmainwindow_->removeToolBar( tb ); }


void uiMainWin::addToolBarBreak()
{ qmainwindow_->addToolBarBreak(); }


uiMenu& uiMainWin::getToolbarsMenu() const
{ return qmainwindow_->getToolbarsMenu(); }


const ObjectSet<uiToolBar>& uiMainWin::toolBars() const
{ return qmainwindow_->toolBars(); }


const ObjectSet<uiDockWin>& uiMainWin::dockWins() const
{ return qmainwindow_->dockWins(); }


uiGroup* uiMainWin::topGroup()
{ return qmainwindow_->uiCentralWidg(); }


void uiMainWin::setShrinkAllowed(bool yn)
    { if ( topGroup() ) topGroup()->setShrinkAllowed(yn); }


bool uiMainWin::shrinkAllowed()
    { return topGroup() ? topGroup()->shrinkAllowed() : false; }


uiObject* uiMainWin::mainobject()
    { return qmainwindow_->uiCentralWidg()->mainObject(); }


void uiMainWin::toStatusBar( const uiString& txt, int fldidx, int msecs )
{
    uiStatusBar* sb = statusBar();
    if ( sb )
	sb->message( txt, fldidx, msecs );
    else if ( txt.isSet() )
	UsrMsg(txt.getFullString());
}


void uiMainWin::setSensitive( bool yn )
{
    if ( menuBar() ) menuBar()->setSensitive( yn );
    qmainwindow_->setEnabled( yn );
}


uiMainWin* uiMainWin::gtUiWinIfIsBdy(QWidget* mwimpl)
{
    if ( !mwimpl ) return 0;

    ODMainWindow* _mwb = dynamic_cast<ODMainWindow*>( mwimpl );
    if ( !_mwb ) return 0;

    return &_mwb->handle();
}


void uiMainWin::setCornerPos( int x, int y )
{ qmainwindow_->move( x, y ); }


uiRect uiMainWin::geometry( bool frame ) const
{
    // Workaround for Qt-bug: top left of area sometimes translates to origin!
    QRect qarea = qmainwindow_->geometry();
    QRect qframe = qmainwindow_->frameGeometry();
    QPoint correction = qmainwindow_->mapToGlobal(QPoint(0,0)) - qarea.topLeft();
    qframe.translate( correction );
    qarea.translate( correction );
    QRect qrect = frame ? qframe : qarea;

    //QRect qrect = frame ? qmainwindow_->frameGeometry() : qmainwindow_->geometry();
    uiRect rect( qrect.left(), qrect.top(), qrect.right(), qrect.bottom() );
    return rect;
}


void uiMainWin::setIcon( const uiPixmap& pm )
{ qmainwindow_->setWindowIcon( *pm.qpixmap() ); }

void uiMainWin::setIconText( const uiString& txt)
{ qmainwindow_->setWindowIconText( txt.getQString() ); }

void uiMainWin::saveSettings()
{ qmainwindow_->saveSettings(); }

void uiMainWin::readSettings()
{ qmainwindow_->readSettings(); }

void uiMainWin::raise()
{ qmainwindow_->raise(); }


void uiMainWin::programActiveWindow( uiMainWin* mw )
{ programmedactivewin_ = mw; }


uiMainWin* uiMainWin::programmedActiveWindow()
{ return programmedactivewin_; }


void uiMainWin::runScript( const char* filename )
{
    scripttorun_ = filename;
    runScriptRequest.trigger();
}


const char* uiMainWin::getScriptToRun() const
{ return scripttorun_; }


uiMainWin* uiMainWin::activeWindow()
{
    if ( programmedactivewin_ )
	return programmedactivewin_;

    QWidget* _aw = qApp->activeWindow();
    if ( !_aw )		return 0;

    ODMainWindow* _awb = dynamic_cast<ODMainWindow*>(_aw);
    if ( !_awb )	return 0;

    return &_awb->handle();
}


uiMainWin::ActModalTyp uiMainWin::activeModalType()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )					return None;

    if ( dynamic_cast<ODMainWindow*>(amw) )	return Main;
    if ( dynamic_cast<QMessageBox*>(amw) )	return Message;
    if ( dynamic_cast<QFileDialog*>(amw) )	return File;
    if ( dynamic_cast<QColorDialog*>(amw) )	return Colour;
    if ( dynamic_cast<QFontDialog*>(amw) )	return Font;

    return Unknown;
}


uiMainWin* uiMainWin::activeModalWindow()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )	return 0;

    ODMainWindow* mwb = dynamic_cast<ODMainWindow*>( amw );
    if ( !mwb )	return 0;

    return &mwb->handle();
}


const char* uiMainWin::activeModalQDlgTitle()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )
	return 0;

    mDeclStaticString( title );
    title = amw->windowTitle();
    return title;
}


static QMessageBox::StandardButton getStandardButton( const QMessageBox* qmb,
						      int buttonnr )
{
    int stdbutcount = 0;

    for ( unsigned int idx=QMessageBox::Ok;
	  qmb && idx<=QMessageBox::RestoreDefaults; idx+=idx )
    {
	if ( !qmb->button((QMessageBox::StandardButton) idx) )
	    continue;

	if ( stdbutcount == buttonnr )
	    return (QMessageBox::StandardButton) idx;

	stdbutcount++;
    }

    return QMessageBox::NoButton;
}


const char* uiMainWin::activeModalQDlgButTxt( int buttonnr )
{
    const ActModalTyp typ = activeModalType();
    QWidget* amw = qApp->activeModalWidget();

    if ( typ == Message )
    {
	const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );
	mDeclStaticString( buttext );

	const QMessageBox::StandardButton stdbut =
					getStandardButton( qmb, buttonnr );
        if ( stdbut )
	    // TODO: get original text if button text is translation
	    buttext = qmb->button(stdbut)->text();
	else
	    buttext = "";

	return buttext;
    }

    if ( typ==Colour || typ==Font || typ==File )
    {
	if ( buttonnr == 0 ) return "Cancel";
	if ( buttonnr == 1 ) return typ==File ? "Ok" : "OK";
	return "";
    }

    return 0;
}


int uiMainWin::activeModalQDlgRetVal( int buttonnr )
{
    QWidget* amw = qApp->activeModalWidget();
    const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );
    const QMessageBox::StandardButton stdbut =
					getStandardButton( qmb, buttonnr );

    return stdbut ? ((int) stdbut) : buttonnr;
}


void uiMainWin::closeActiveModalQDlg( int retval )
{
    if ( activeModalWindow() )
	return;

    QWidget* _amw = qApp->activeModalWidget();
    if ( !_amw )
	return;

    QDialog* _qdlg = dynamic_cast<QDialog*>(_amw);
    if ( !_qdlg )
	return;

    _qdlg->done( retval );
}


void uiMainWin::getTopLevelWindows( ObjectSet<uiMainWin>& windowlist,
				    bool visibleonly )
{
    windowlist.erase();
    winlistmutex_.lock();
    for ( int idx=0; idx<orderedwinlist_.size(); idx++ )
    {
	if ( !visibleonly || !orderedwinlist_[idx]->isHidden() )
	    windowlist += orderedwinlist_[idx];
    }
    winlistmutex_.unLock();
}


void uiMainWin::getModalSignatures( BufferStringSet& signatures )
{
    signatures.erase();
    QWidgetList toplevelwigs = qApp->topLevelWidgets();

    for ( int idx=0; idx<toplevelwigs.count(); idx++ )
    {
	const QWidget* qw = toplevelwigs.at( idx );
	if ( qw->isWindow() && !qw->isHidden() && qw->isModal() )
	{
	    BufferString qwptrstr;
	    sprintf( qwptrstr.getCStr(), "%p", qw );
	    signatures.add( qwptrstr );
	}
    }
}


uiString uiMainWin::uniqueWinTitle( const uiString& txt,
				    QWidget* forwindow,
				    BufferString* outputaddendum )
{
    const QWidgetList toplevelwigs = qApp->topLevelWidgets();

    uiString res;
    for ( int count=1; true; count++ )
    {
	bool unique = true;
	uiString beginning = txt.isEmpty() ? tr("<no title>") : txt;

	if ( count>1 )
	{
	    res = toUiString( "%1 {%2}").arg( beginning )
		                        .arg( toUiString(count) );
	    BufferString addendum( "  {", toString(count), "}" );
	    if ( outputaddendum ) *outputaddendum = addendum;
	}
	else
	{
	    res = beginning;
	}

	QString wintitle = res.getQString();

	for ( int idx=0; idx<toplevelwigs.count(); idx++ )
	{
	    const QWidget* qw = toplevelwigs.at( idx );
	    if ( !qw->isWindow() || qw->isHidden() || qw==forwindow )
		continue;

	    if ( qw->windowTitle() == wintitle )
	    {
		unique = false;
		break;
	    }
	}

	if ( unique ) break;
    }

    return res;
}


bool uiMainWin::grab( const char* filenm, int zoom,
		      const char* format, int quality ) const
{
    const WId desktopwinid = QApplication::desktop()->winId();
    const QPixmap desktopsnapshot = QPixmap::grabWindow( desktopwinid );
    QPixmap snapshot = desktopsnapshot;
    if ( zoom > 0 )
    {
	QWidget* qwin = qApp->activeModalWidget();
	if ( !qwin || zoom==1 )
	    qwin = qmainwindow_;

	const int wdth = qwin->frameGeometry().width();
	const int hght = qwin->frameGeometry().height();
	snapshot = desktopsnapshot.copy( qwin->x(), qwin->y(), wdth, hght );
    }

    return snapshot.save( QString(filenm), format, quality );
}


void uiMainWin::activateInGUIThread( const CallBack& cb, bool busywait )
{ qmainwindow_->activateInGUIThread( cb, busywait ); }


void uiMainWin::translateText()
{
    uiParent::translateText();

    for ( int idx=0; idx<qmainwindow_->toolbars_.size(); idx++ )
	qmainwindow_->toolbars_[idx]->translateText();

    //Don't know if anything special needs to be done here.
}


void uiMainWin::copyToClipBoardCB( CallBacker* )
{
    copyToClipBoard();
}


void uiMainWin::languageChangeCB( CallBacker* )
{
    if ( languagechangecount_<TrMgr().changeCount() )
    {
        translateText();
	languagechangecount_ = TrMgr().changeCount();
    }
}


void uiMainWin::aftPopupCB( CallBacker* )
{
    afterPopup.trigger();
}



class ImageSaver : public CallBacker
{ mODTextTranslationClass(ImageSaver)
public:

ImageSaver()
{
    timer_.tick.notify( mCB(this,ImageSaver,shootImageCB) );
}


void setImageProp( WId qwid, int w, int h, int r )
{
    qwinid_ = qwid;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = true;
    timer_.start( 500, true );
}


void setImageProp( WId qwid, const char* fnm, int w, int h, int r )
{
    qwinid_ = qwid;
    fname_ = fnm;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = false;
    timer_.start( 500, true );
}


protected:
void shootImageCB( CallBacker* )
{
    const QPixmap snapshot = QPixmap::grabWindow( qwinid_ );

    QImage image = snapshot.toImage();
    image = image.scaledToWidth( width_ );
    image = image.scaledToHeight( height_ );
    image.setDotsPerMeterX( (int)(res_/0.0254) );
    image.setDotsPerMeterY( (int)(res_/0.0254) );
    if ( copytoclipboard_ )
	uiClipboard::setImage( image );
    else
	image.save( fname_ );

    timer_.stop();
}

    int		width_;
    int		height_;
    int		res_;
    bool	copytoclipboard_;
    QString	fname_;
    WId		qwinid_;
    Timer	timer_;
};


void uiMainWin::copyToClipBoard()
{
    QWidget* qwin = getWidget(0);
    if ( !qwin )
	qwin = qmainwindow_;
    WId wid = qwin->winId();
    const int wdth = qwin->frameGeometry().width();
    const int hght = qwin->frameGeometry().height();
    const int dpi = uiMain::getDPI();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( wid, wdth, hght, dpi );
}


void uiMainWin::saveImage( const char* fnm, int wdth, int hght, int res )
{
    QWidget* qwin = getWidget(0);
    if ( !qwin )
	qwin = qmainwindow_;
    WId wid = qwin->winId();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( wid, fnm, wdth, hght, res );
}


void uiMainWin::saveAsPDF_PS( const char* filename, bool aspdf, int w,
				    int h, int res )
{
    QString fileName( filename );
    QPrinter* pdfprinter = new QPrinter();
#if QT_VERSION >= 0x050000
    pdfprinter->setOutputFormat( QPrinter::PdfFormat );
#else
    pdfprinter->setOutputFormat( aspdf ? QPrinter::PdfFormat
				       : QPrinter::PostScriptFormat );
#endif
    pdfprinter->setPaperSize( QSizeF(w,h), QPrinter::Point );
    pdfprinter->setFullPage( false );
    pdfprinter->setOutputFileName( filename );
    pdfprinter->setResolution( res );

    QPainter* pdfpainter = new QPainter();
    pdfpainter->begin( pdfprinter );
    QWidget* qwin = getWidget(0);
    qwin->render( pdfpainter, pdfprinter->pageRect().topLeft(), qwin->rect() );
    pdfpainter->end();
    delete pdfpainter;
    delete pdfprinter;
}




//============================================================================

/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.
*/


#define mHandle static_cast<uiDialog&>(handle_)

class uiDialogBody : public ODMainWindow
{ mODTextTranslationClass(uiDialogBody);
public:
			uiDialogBody(uiDialog&,uiParent*,
				     const uiDialog::Setup&);
			~uiDialogBody();

    int			exec( bool showminimized );

    void		reject( CallBacker* s )
			{
			    mHandle.cancelpushed_ = s == cnclbut_;
			    if ( mHandle.rejectOK(s) )
				_done(0);
			    else
				uiSetResult( -1 );
			}
                        //!< to be called by a 'cancel' button
    void		accept( CallBacker* s )
			    { if ( mHandle.acceptOK(s) ) _done(1); }
                        //!< to be called by a 'ok' button
    void		done( int i )
			    { if ( mHandle.doneOK(i) ) _done(i); }

    void		uiSetResult( int v )	{ result_ = v; }
    int			uiResult()		{ return result_; }

    void		setTitleText(const uiString& txt);
    void		setOkCancelText(const uiString&,const uiString&);
    void		setOkText(const uiString&);
			//!< OK button disabled when set to empty
    void		setCancelText(const uiString&);
			//!< Cancel button disabled when set to empty
    void		setApplyText(const uiString&);
    void		enableSaveButton(const uiString& txt);
    void		setSaveButtonChecked(bool yn);
    void		setButtonSensitive(uiDialog::Button,bool yn);
    bool		saveButtonChecked() const;
    bool		hasSaveButton() const;
    uiButton*		button(uiDialog::Button);

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn )	{ setup_.separator_ = yn; }
    bool		separator() const	{ return setup_.separator_; }
    void		setHelpKey(const HelpKey& key) { setup_.helpkey_ = key;}
    HelpKey		helpKey() const	{ return setup_.helpkey_; }

    void		setDlgGrp( uiGroup* cw )	{ dlggrp_=cw; }
    uiGroup*		getDlgGrp() 			{ return dlggrp_; }

    void		setHSpacing( int spc )	{ dlggrp_->setHSpacing(spc); }
    void		setVSpacing( int spc )	{ dlggrp_->setVSpacing(spc); }
    void		setBorder( int b )	{ dlggrp_->setBorder( b ); }

    virtual void        addChild(uiBaseObject& child);
    virtual void        manageChld_(uiBaseObject&,uiObjectBody&);
    virtual void	attachChild(constraintType,uiObject* child,
				    uiObject* other,int margin,bool reciprocal);
    void		provideHelp(CallBacker*);
    void		showCredits(CallBacker*);

    const uiDialog::Setup& getSetup() const	{ return setup_; }

protected:

    virtual const QWidget* managewidg_() const
			{
			    if ( !initing_ )
				return dlggrp_->pbody()->managewidg();
			    return ODMainWindow::managewidg_();
			}

    int			result_;
    bool		initchildrendone_;

    uiGroup*            dlggrp_;
    uiDialog::Setup	setup_;

    uiButton*		okbut_;
    uiButton*		cnclbut_;
    uiButton*		applybut_;
    uiButton*		helpbut_;
    uiToolButton*	creditsbut_;

    uiCheckBox*		savebutcb_;
    uiToolButton*	savebuttb_;

    uiLabel*		titlelbl_;

    void		_done(int);

    virtual void	finalise()	{ finalise(false); }
    virtual void	finalise(bool);
    void		closeEvent(QCloseEvent*);
    void		applyCB(CallBacker*);

private:

    void		initChildren();
    uiObject*		createChildren();
    void		layoutChildren(uiObject*);
    void		layoutChildrenOld(uiObject*);

};


uiDialogBody::uiDialogBody( uiDialog& hndle, uiParent* parnt,
			    const uiDialog::Setup& s )
    : ODMainWindow(hndle,parnt,s.wintitle_.getFullString(),s.modal_)
    , dlggrp_(0)
    , setup_(s)
    , okbut_(0), cnclbut_(0), applybut_(0)
    , savebutcb_(0),  savebuttb_(0)
    , helpbut_(0), creditsbut_(0)
    , titlelbl_(0), result_(0)
    , initchildrendone_(false)
{
    setContentsMargins( 10, 2, 10, 2 );
}


uiDialogBody::~uiDialogBody()
{
    if ( okbut_ )
	okbut_->activated.remove( mCB(this,uiDialogBody,accept) );

    if ( cnclbut_ )
	cnclbut_->activated.remove( mCB(this,uiDialogBody,reject) );
}


int uiDialogBody::exec( bool showminimized )
{
    uiSetResult( 0 );

    if ( setup_.fixedsize_ )
	setSizePolicy( QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed) );

    move( handle_.getPopupArea() );
    go( showminimized );

    return uiResult();
}


void uiDialogBody::setTitleText( const uiString& txt )
{
    setup_.dlgtitle_ = txt;
    if ( titlelbl_ )
	titlelbl_->setText( txt );
}


void uiDialogBody::setOkCancelText( const uiString& oktxt,
				    const uiString& cncltxt )
{
    setOkText( oktxt );
    setCancelText( cncltxt );
}


void uiDialogBody::setOkText( const uiString& txt )
{
    setup_.oktext_ = txt;
    if ( okbut_ ) okbut_->setText(txt);
}


void uiDialogBody::setCancelText( const uiString& txt )
{
    setup_.canceltext_ = txt;
    if ( cnclbut_ ) cnclbut_->setText( txt );
}


void uiDialogBody::setApplyText( const uiString& txt )
{ if ( applybut_ ) applybut_->setText( txt ); }


bool uiDialogBody::hasSaveButton() const
{ return savebutcb_; }


bool uiDialogBody::saveButtonChecked() const
{ return savebutcb_ ? savebutcb_->isChecked() : false; }


//! Hides the box, which also exits the event loop in case of a modal box.
void uiDialogBody::_done( int v )
{
    uiSetResult( v );
    close();
}


void uiDialogBody::closeEvent( QCloseEvent* ce )
{
    const int refnr = handle_.beginCmdRecEvent( "Close" );

    reject(0);
    if ( result_ == -1 )
	ce->ignore();
    else
	ce->accept();

    handle_.endCmdRecEvent( refnr, "Close" );
}


void uiDialogBody::enableSaveButton( const uiString& txt )
{ setup_.savetext_ = txt; setup_.savebutton_ = true; }

void uiDialogBody::setSaveButtonChecked( bool yn )
{
    setup_.savechecked_ = yn;
    if ( savebutcb_ ) savebutcb_->setChecked(yn);
}


void uiDialogBody::setButtonSensitive( uiDialog::Button but, bool yn )
{
    switch ( but )
    {
    case uiDialog::OK:		if ( okbut_ ) okbut_->setSensitive(yn);
    break;
    case uiDialog::CANCEL:	if ( cnclbut_ ) cnclbut_->setSensitive(yn);
    break;
    case uiDialog::APPLY:	if ( applybut_ ) applybut_->setSensitive(yn);
    break;
    case uiDialog::HELP:	if ( helpbut_ ) helpbut_->setSensitive(yn);
    break;
    case uiDialog::SAVE:
	if ( savebutcb_ ) savebutcb_->setSensitive(yn);
	if ( savebuttb_ ) savebuttb_->setSensitive(yn);
    break;
    case uiDialog::CREDITS:
	if ( creditsbut_ ) creditsbut_->setSensitive(yn);
    break;
    }
}


uiButton* uiDialogBody::button( uiDialog::Button but )
{
    switch ( but )
    {
    case uiDialog::OK:		return okbut_; break;
    case uiDialog::CANCEL:	return cnclbut_; break;
    case uiDialog::APPLY:	return applybut_; break;
    case uiDialog::HELP:	return helpbut_; break;
    case uiDialog::SAVE:
	return savebutcb_
	    ? (uiButton*)savebutcb_ : (uiButton*)savebuttb_;
    break;
    case uiDialog::CREDITS:	return creditsbut_; break;
    }

    return 0;
}


void uiDialogBody::addChild( uiBaseObject& child )
{
    if ( !initing_ )
	dlggrp_->addChild( child );
    else
	ODMainWindow::addChild( child );
}


void uiDialogBody::manageChld_( uiBaseObject& o, uiObjectBody& b )
{
    if ( !initing_ )
	dlggrp_->manageChld( o, b );
}


void uiDialogBody::attachChild( constraintType tp, uiObject* child,
				uiObject* other, int margin, bool reciprocal )
{
    if ( !child || initing_ ) return;

    dlggrp_->attachChild( tp, child, other, margin, reciprocal );
}



//  Construct OK and Cancel buttons just before the first show.
//  This gives chance not to construct them in case OKtext and CancelText have
//  been set to ""

void uiDialogBody::finalise( bool )
{
    ODMainWindow::finalise( false );

    handle_.preFinalise().trigger( handle_ );

    dlggrp_->finalise();

    if ( !initchildrendone_ )
	initChildren();

    finaliseChildren();

    handle_.postFinalise().trigger( handle_ );
}


void uiDialogBody::initChildren()
{
    uiObject* lowestobject = createChildren();
    if ( GetEnvVarYN("DTECT_OLD_BUTTON_LAYOUT") )
	layoutChildrenOld( lowestobject );
    else
	layoutChildren( lowestobject );

    if ( okbut_ )
    {
	okbut_->activated.notify( mCB(this,uiDialogBody,accept) );
	mDynamicCastGet(uiPushButton*,pb,okbut_)
	if ( pb )
	    pb->setDefault();
    }
    if ( cnclbut_ )
    {
	cnclbut_->activated.notify( mCB(this,uiDialogBody,reject) );
	if ( !okbut_ )
	{
	    mDynamicCastGet(uiPushButton*,pb,cnclbut_)
	    if ( pb )
		pb->setDefault();
	}
    }

    initchildrendone_ = true;
}


uiObject* uiDialogBody::createChildren()
{
    if ( !setup_.oktext_.isEmpty() )
	okbut_ = uiButton::getStd( centralwidget_, OD::Ok, CallBack(),
				   true, setup_.oktext_ );
    if ( !setup_.canceltext_.isEmpty() )
	cnclbut_ = uiButton::getStd( centralwidget_, OD::Cancel,
				     CallBack(), true, setup_.canceltext_ );
    if ( setup_.applybutton_ )
	applybut_ = uiButton::getStd( centralwidget_, OD::Apply,
				mCB(this,uiDialogBody,applyCB), true,
				   setup_.applytext_ );

    if ( setup_.savebutton_ && !setup_.savetext_.isEmpty() )
    {
	if ( setup_.savebutispush_ )
	    savebuttb_ = new uiToolButton( centralwidget_, "save",
			  setup_.savetext_, CallBack() );
	else
	{
	    savebutcb_ = new uiCheckBox( centralwidget_, setup_.savetext_ );
	    savebutcb_->setChecked( setup_.savechecked_ );
	}
    }
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    if ( !dlg.helpKey().isEmpty() )
    {
	mDefineStaticLocalObject( bool, shwhid,
				  = GetEnvVarYN("DTECT_SHOW_HELP") );
#ifdef __debug__
	shwhid = true;
#endif

	helpbut_ = uiButton::getStd( centralwidget_, OD::Help,
				mCB(this,uiDialogBody,provideHelp), true );
	if ( shwhid )
	    helpbut_->setToolTip( uiStrings::phrJoinStrings(
	    toUiString(dlg.helpKey().providername_),
	    mToUiStringTodo(dlg.helpKey().argument_)) );
	else
	    helpbut_->setToolTip( tr("Help on this window") );
    }

    if ( !setup_.menubar_ && !setup_.dlgtitle_.isEmpty() )
    {
	titlelbl_ = new uiLabel( centralwidget_, setup_.dlgtitle_ );
	titlelbl_->setHSzPol( uiObject::WideVar );
	uiObject* obj = setup_.separator_
			    ? (uiObject*) new uiSeparator(centralwidget_)
			    : (uiObject*) titlelbl_;

	if ( obj != titlelbl_ )
	{
	    if ( uiDialog::titlePos() == 0 )
		titlelbl_->attach( centeredAbove, obj );
	    else if ( uiDialog::titlePos() > 0 )
		titlelbl_->attach( rightBorder );
	    obj->attach( stretchedBelow, titlelbl_, -2 );
	}
	if ( setup_.mainwidgcentered_ )
	    dlggrp_->attach( centeredBelow, obj );
	else
	    dlggrp_->attach( stretchedBelow, obj );
    }

    uiObject* lowestobj = dlggrp_->mainObject();
    if ( setup_.separator_ && ( okbut_ || cnclbut_ || savebutcb_ ||
			       savebuttb_ || helpbut_) )
    {
	uiSeparator* horSepar = new uiSeparator( centralwidget_ );
	horSepar->attach( stretchedBelow, dlggrp_, -2 );
	lowestobj = horSepar;
    }

    return lowestobj;
}


static const int hborderdist = 1;
static const int vborderdist = 5;

static void attachButton( uiObject* but, uiObject*& prevbut,
			  uiObject* lowestobj )
{
    if ( !but ) return;

    but->attach( ensureBelow, lowestobj );
    but->attach( bottomBorder, vborderdist );
    if ( prevbut )
	but->attach( leftOf, prevbut );
    else
	but->attach( rightBorder, hborderdist );

    prevbut = but;
}


void uiDialogBody::layoutChildren( uiObject* lowestobj )
{
    uiObject* leftbut = setup_.okcancelrev_ ? cnclbut_ : okbut_;
    uiObject* rightbut = setup_.okcancelrev_ ? okbut_ : cnclbut_;

    uiObject* prevbut = 0;
    attachButton( helpbut_, prevbut, lowestobj );
    attachButton( applybut_, prevbut, lowestobj );
    attachButton( rightbut, prevbut, lowestobj );
    attachButton( leftbut, prevbut, lowestobj );
    attachButton( creditsbut_, prevbut, lowestobj );

    uiObject* savebut = savebutcb_;
    if ( !savebut ) savebut = savebuttb_;
    if ( savebut )
    {
	savebut->attach( ensureBelow, lowestobj );
	savebut->attach( bottomBorder, vborderdist );
	savebut->attach( leftBorder, hborderdist );
	if ( prevbut )
	    savebut->attach( ensureLeftOf, prevbut );
    }
}


void uiDialogBody::layoutChildrenOld( uiObject* lowestobj )
{
    uiObject* leftbut = setup_.okcancelrev_ ? cnclbut_ : okbut_;
    uiObject* rightbut = setup_.okcancelrev_ ? okbut_ : cnclbut_;
    uiObject* exitbut = okbut_ ? okbut_ : cnclbut_;
    uiObject* centerbut = helpbut_;
    uiObject* extrabut = savebuttb_;

    if ( !okbut_ || !cnclbut_ )
    {
	leftbut = rightbut = 0;
	if ( exitbut )
	{
	    centerbut = exitbut;
	    extrabut = helpbut_;
	    leftbut = savebuttb_;
	}
    }

    if ( !centerbut )
    {
	centerbut = extrabut;
	extrabut = 0;
    }

#define mCommonLayout(but) \
    but->attach( ensureBelow, lowestobj ); \
    but->attach( bottomBorder, vborderdist )

    if ( leftbut )
    {
	mCommonLayout(leftbut);
	leftbut->attach( leftBorder, hborderdist );
    }

    if ( rightbut )
    {
	mCommonLayout(rightbut);
	rightbut->attach( rightBorder, hborderdist );
	if ( leftbut )
	    rightbut->attach( ensureRightOf, leftbut );
    }

    if ( centerbut )
    {
	mCommonLayout(centerbut);
	centerbut->attach( hCentered );
	if ( leftbut )
	    centerbut->attach( ensureRightOf, leftbut );
	if ( rightbut )
	    centerbut->attach( ensureLeftOf, rightbut );
    }

    if ( savebutcb_ )
    {
	savebutcb_->attach( extrabut ? leftOf : rightOf, exitbut );
	if ( centerbut && centerbut != exitbut )
	    centerbut->attach( ensureRightOf, savebutcb_ );
	if ( rightbut && rightbut != exitbut )
	    rightbut->attach( ensureRightOf, savebutcb_ );
    }

    if ( extrabut )
	extrabut->attach( rightOf, centerbut );
}


void uiDialogBody::provideHelp( CallBacker* )
{
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    HelpProvider::provideHelp( dlg.helpKey() );
}


void uiDialogBody::applyCB( CallBacker* cb )
{
    mDynamicCastGet(uiDialog&,dlg,handle_);
    dlg.applyPushed.trigger( cb );
}


//====== uiDialog =============================================================


#define mBody static_cast<uiDialogBody*>(body_)

uiDialog::uiDialog( uiParent* p, const uiDialog::Setup& s )
    : uiMainWin( s.wintitle_, p )
    , cancelpushed_(false)
    , applyPushed(this)
{
    body_= new uiDialogBody( *this, p, s );
    setBody( body_ );
    body_->construct( s.nrstatusflds_, s.menubar_ );
    uiGroup* cw = new uiGroup( body_->uiCentralWidg(), "Dialog central widget");
    cw->setStretch( 2, 2 );
    mBody->setDlgGrp( cw );
    setTitleText( s.dlgtitle_ );
    ctrlstyle_ = OkAndCancel;
}


void uiDialog::setButtonText( Button but, const uiString& txt )
{
    switch ( but )
    {
        case OK		: setOkText( txt ); break;
        case CANCEL	: setCancelText( txt ); break;
	case APPLY	: mBody->setApplyText( txt ); break;
        case HELP	: pErrMsg("set help txt but"); break;
	case SAVE	: enableSaveButton( txt ); break;
        case CREDITS	: pErrMsg("set credits txt but"); break;
    }
}


void uiDialog::setCtrlStyle( uiDialog::CtrlStyle cs )
{
    uiString oktext = toUiString("Run");
    uiString canceltext = uiStrings::sClose();
    if ( GetEnvVarYN("DTECT_OLD_BUTTON_LAYOUT") )
    {
	oktext = uiStrings::sGo();
	canceltext = uiStrings::sCancel();
    }

    switch ( cs )
    {
    case OkAndCancel:
	setOkCancelText( uiStrings::sOk(), uiStrings::sCancel() );
    break;
    case RunAndClose:
	setOkCancelText( oktext, canceltext );
    break;
    case CloseOnly:
	    setOkCancelText(
		mBody->finalised() ? canceltext : uiString::emptyString(),
		canceltext );
    break;
    }

    ctrlstyle_ = cs;
}


void uiDialog::showMinMaxButtons()
{
    Qt::WindowFlags flags = body_->windowFlags();
    flags |= Qt::WindowMinMaxButtonsHint;
    body_->setWindowFlags( flags );
}


void uiDialog::showAlwaysOnTop()
{
    Qt::WindowFlags flags = body_->windowFlags();
    flags |= Qt::WindowStaysOnTopHint;
    body_->setWindowFlags( flags );
}


int uiDialog::go()
{
    addToOrderedWinList( this );
    return mBody->exec( false );
}


int uiDialog::goMinimized()
{
    addToOrderedWinList( this );
    return mBody->exec( true );
}


const uiDialog::Setup& uiDialog::setup() const	{ return mBody->getSetup(); }
void uiDialog::reject( CallBacker* cb)		{ mBody->reject( cb ); }
void uiDialog::accept( CallBacker*cb)		{ mBody->accept( cb ); }
void uiDialog::done( int i )			{ mBody->done( i ); }
void uiDialog::setHSpacing( int s )		{ mBody->setHSpacing(s); }
void uiDialog::setVSpacing( int s )		{ mBody->setVSpacing(s); }
void uiDialog::setBorder( int b )		{ mBody->setBorder(b); }
void uiDialog::setTitleText(const uiString& txt){ mBody->setTitleText(txt); }
void uiDialog::setOkText( const uiString& txt ) { mBody->setOkText(txt); }
void uiDialog::setCancelText(const uiString& t) { mBody->setCancelText(t);}
void uiDialog::enableSaveButton(const uiString& t){ mBody->enableSaveButton(t);}
uiButton* uiDialog::button(Button b)		{ return mBody->button(b); }
void uiDialog::setSeparator( bool yn )		{ mBody->setSeparator(yn); }
bool uiDialog::separator() const		{ return mBody->separator(); }
void uiDialog::setHelpKey( const HelpKey& key ) { mBody->setHelpKey(key); }
HelpKey uiDialog::helpKey() const		{ return mBody->helpKey(); }
int uiDialog::uiResult() const			{ return mBody->uiResult(); }
void uiDialog::setModal( bool yn )		{ mBody->setModal( yn ); }
bool uiDialog::isModal() const			{ return mBody->isModal(); }

void uiDialog::setOkCancelText( const uiString& oktxt, const uiString& cncltxt )
{ mBody->setOkCancelText( oktxt, cncltxt ); }

void uiDialog::setButtonSensitive(uiDialog::Button b, bool s )
    { mBody->setButtonSensitive(b,s); }
void uiDialog::setSaveButtonChecked(bool b)
    { mBody->setSaveButtonChecked(b); }
bool uiDialog::saveButtonChecked() const
    { return mBody->saveButtonChecked(); }
bool uiDialog::hasSaveButton() const
    { return mBody->hasSaveButton(); }

const uiLayoutMgr* uiDialog::getLayoutMgr() const
{
    return const_cast<const uiGroup*>(mBody->getDlgGrp())->getLayoutMgr();
}

int uiDialog::titlepos_ = 0; // default is centered.
int uiDialog::titlePos()			{ return titlepos_; }
void uiDialog::setTitlePos( int p )		{ titlepos_ = p; }

#endif
