/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiflatviewstdcontrol.cc,v 1.28 2009-09-03 09:52:46 cvssatyaki Exp $";

#include "uiflatviewstdcontrol.h"

#include "uiflatviewcoltabed.h"
#include "flatviewzoommgr.h"
#include "uiflatviewer.h"
#include "uiflatviewthumbnail.h"
#include "uigraphicsscene.h"
#include "uibutton.h"
#include "uimainwin.h"
#include "uimenuhandler.h"
#include "uirgbarraycanvas.h"
#include "uitoolbar.h"
#include "uiworld2ui.h"

#include "keyboardevent.h"
#include "mouseevent.h"
#include "pixmap.h"

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, 0, ioPixmap(fnm), \
			    mCB(this,uiFlatViewStdControl,cbnm) ); \
    but->setToolTip( tt ); \
    tb_->addObject( but );

uiFlatViewStdControl::uiFlatViewStdControl( uiFlatViewer& vwr,
					    const Setup& setup )
    : uiFlatViewControl(vwr,setup.parent_,true,setup.withwva_)
    , vwr_(vwr)
    , ctabed_(0)
    , manip_(false)
    , mousepressed_(false)
    , viewdragged_(false)
    , menu_(*new uiMenuHandler(&vwr,-1))	//TODO multiple menus ?
    , propertiesmnuitem_("Properties...",100)
    , manipdrawbut_(0)
    , editbut_(0)
{
    tb_ = new uiToolBar( mainwin(), "Flat Viewer Tools",
	    setup.withcoltabed_ ? uiToolBar::Left : uiToolBar::Top );
    if ( setup.withstates_ )
    {
	mDefBut(manipdrawbut_,"altpick.png",stateCB,"Switch view mode");
	vwr_.rgbCanvas().getKeyboardEventHandler().keyPressed.notify(
		mCB(this,uiFlatViewStdControl,keyPressCB) );
    }

    vwr_.setRubberBandingOn( !manip_ );

    if ( setup.withedit_ )
    {
	mDefBut(editbut_,"seedpickmode.png",editCB,"Edit mode");
	editbut_->setToggleButton( true );
    }

    mDefBut(zoominbut_,"zoomforward.png",zoomCB,"Zoom in");
    mDefBut(zoomoutbut_,"zoombackward.png",zoomCB,"Zoom out");
    uiToolButton* mDefBut(fliplrbut,"flip_lr.png",flipCB,"Flip left/right");
    tb_->addObject( vwr_.rgbCanvas().getSaveImageButton() );

    tb_->addSeparator();
    mDefBut(parsbut_,"2ddisppars.png",parsCB,"Set display parameters");

    if ( setup.withcoltabed_ )
    {
	uiToolBar* coltabtb = new uiToolBar( mainwin(), "Color Table" );
	ctabed_ = new uiFlatViewColTabEd( this, vwr );
	ctabed_->colTabChgd.notify( mCB(this,uiFlatViewStdControl,coltabChg) );
	coltabtb->addObject( ctabed_->colTabGrp()->attachObj() );
	coltabtb->display( vwr.rgbCanvas().prefHNrPics()>400 );
    }

    if ( !setup.helpid_.isEmpty() )
    {
	uiToolButton* mDefBut(helpbut,"contexthelp.png",helpCB,"Help");
	helpid_ = setup.helpid_;
    }

    vwr.viewChanged.notify( mCB(this,uiFlatViewStdControl,vwChgCB) );
    vwr.dispParsChanged.notify( mCB(this,uiFlatViewStdControl,dispChgCB) );

    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiFlatViewStdControl,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiFlatViewStdControl,handleMenuCB));

    new uiFlatViewThumbnail( this, vwr );
    //TODO attach keyboard events to panCB
}


uiFlatViewStdControl::~uiFlatViewStdControl()
{
    menu_.unRef();
}


void uiFlatViewStdControl::finalPrepare()
{
    updatePosButtonStates();
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	MouseEventHandler& mevh =
	    vwrs_[vwrs_.size()-1]->rgbCanvas().getNavigationMouseEventHandler();
	mevh.wheelMove.notify( mCB(this,uiFlatViewStdControl,wheelMoveCB) );
	if ( vwrs_[idx]->hasHandDrag() )
	{
	    mevh.buttonPressed.notify(
		    mCB(this,uiFlatViewStdControl,handDragStarted));
	    mevh.buttonReleased.notify(
		    mCB(this,uiFlatViewStdControl,handDragged));
	    mevh.movement.notify( mCB(this,uiFlatViewStdControl,handDragging));
	}
    }
}


void uiFlatViewStdControl::updatePosButtonStates()
{
    zoomoutbut_->setSensitive( !zoommgr_.atStart() );
}


void uiFlatViewStdControl::dispChgCB( CallBacker* )
{
    if ( ctabed_ )
	ctabed_->setColTab( vwr_ );
}


void uiFlatViewStdControl::vwChgCB( CallBacker* )
{
    updatePosButtonStates();
}


void uiFlatViewStdControl::wheelMoveCB( CallBacker* )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( !vwrs_[idx]->rgbCanvas().
		getNavigationMouseEventHandler().hasEvent() )
	    continue;

	const MouseEvent& ev =
	    vwrs_[idx]->rgbCanvas().getNavigationMouseEventHandler().event();
	if ( mIsZero(ev.angle(),0.01) )
	    continue;

	zoomCB( ev.angle() < 0 ? zoominbut_ : zoomoutbut_ );
    }
}


void uiFlatViewStdControl::zoomCB( CallBacker* but )
{
    const bool zoomin = but == zoominbut_;
    uiRect viewrect = vwrs_[0]->annotBorder().getRect(
	    vwrs_[0]->rgbCanvas().getViewArea() );
    uiSize newrectsz = viewrect.size();
    if ( but == zoominbut_ )
    {
	zoommgr_.forward();
	newrectsz.setWidth( mNINT(newrectsz.width()*zoommgr_.fwdFac()) );
	newrectsz.setHeight( mNINT(newrectsz.height()*zoommgr_.fwdFac()));
    }
    else
    {
	if ( zoommgr_.atStart() )
	    return;
	zoommgr_.back();
    }

    Geom::Point2D<double> centre;
    Geom::Size2D<double> newsz;
    if ( !vwrs_[0]->rgbCanvas().getNavigationMouseEventHandler().hasEvent() ||
	 !zoomin )
    {
	newsz = zoommgr_.current();
	centre = vwrs_[0]->curView().centre();
    }
    else
    {
	uiWorld2Ui w2ui;
	vwrs_[0]->getWorld2Ui( w2ui );
	const Geom::Point2D<int> viewevpos =
	    vwrs_[0]->rgbCanvas().getCursorPos();
	uiRect selarea( viewevpos.x-(newrectsz.width()/2),
			viewevpos.y-(newrectsz.height()/2),
			viewevpos.x+(newrectsz.width()/2),
			viewevpos.y+(newrectsz.height()/2) );
	int hoffs = 0;
	int voffs = 0;
	if ( viewrect.left() > selarea.left() )
	    hoffs = viewrect.left() - selarea.left();
	if ( viewrect.right() < selarea.right() )
	    hoffs = viewrect.right() - selarea.right();
	if ( viewrect.top() > selarea.top() )
	    voffs = viewrect.top() - selarea.top();
	if ( viewrect.bottom() < selarea.bottom() )
	    voffs = viewrect.bottom() - selarea.bottom();

	selarea += uiPoint( hoffs, voffs );
	uiWorldRect wr = w2ui.transform( selarea );
	centre = wr.centre();
	newsz = wr.size();
    }

    if ( zoommgr_.atStart() )
	centre = zoommgr_.initialCenter();

    setNewView( centre, newsz );
}


void uiFlatViewStdControl::handDragStarted( CallBacker* )
{ mousepressed_ = true; }


void uiFlatViewStdControl::handDragging( CallBacker* )
{
    const uiRGBArrayCanvas& canvas = vwrs_[0]->rgbCanvas();
    if ( (canvas.dragMode() != uiGraphicsViewBase::ScrollHandDrag) ||
	 !mousepressed_ || !vwrs_[0]->hasHandDrag() )
	return;

    viewdragged_ = true;
    Geom::Point2D<double> centre;
    Geom::Size2D<double> newsz;
    uiWorld2Ui w2ui;
    vwrs_[0]->getWorld2Ui( w2ui );

    uiRect viewarea = getViewRect( vwrs_[0] );
    uiWorldRect wr = w2ui.transform( viewarea );
    centre = wr.centre();
    newsz = vwrs_[0]->curView().size();
    wr = getNewWorldRect( centre, newsz, vwrs_[0]->curView(), getBoundingBox());
    if ( wr == vwrs_[0]->curView() )
	return;
    vwrs_[0]->drawAnnot( viewarea, wr );
    vwrs_[0]->viewChanging.trigger( wr );
}


void uiFlatViewStdControl::handDragged( CallBacker* )
{
    mousepressed_ = false;
    const uiRGBArrayCanvas& canvas = vwrs_[0]->rgbCanvas();
    if ( canvas.dragMode() != uiGraphicsViewBase::ScrollHandDrag ||
	 !vwrs_[0]->hasHandDrag() || !viewdragged_ )
	return;
    viewdragged_ = false;
    Geom::Point2D<double> centre;
    Geom::Size2D<double> newsz;
    uiWorld2Ui w2ui;
    vwrs_[0]->getWorld2Ui( w2ui );

    uiRect viewarea = getViewRect( vwrs_[0] );
    uiWorldRect wr = w2ui.transform( viewarea );
    centre = wr.centre();
    if ( centre == vwrs_[0]->curView().centre() )
	return;
    newsz = vwrs_[0]->curView().size();

    setNewView( centre, newsz );
}


void uiFlatViewStdControl::panCB( CallBacker* )
{
    const bool isleft = true;
    const bool isright = false;
    const bool isup = false;
    const bool isdown = false;
    const bool ishor = isleft || isright;

    const uiWorldRect cv( vwrs_[0]->curView() );
    const bool isrev = ishor ? cv.left() > cv.right() : cv.bottom() > cv.top();
    const double fac = 1 - zoommgr_.fwdFac();
    const double pandist = fac * (ishor ? cv.width() : cv.height());

    Geom::Point2D<double> centre = cv.centre();
    Geom::Size2D<double> sz = cv.size();
    if ( (!isrev && isleft) || (isrev && isright) )
	centre.x -= pandist;
    else if ( (isrev && isleft) || (!isrev && isright) )
	centre.x += pandist;
    else if ( (isrev && isup) || (!isrev && isdown) )
	centre.y -= pandist;
    else if ( (!isrev && isup) || (isrev && isdown) )
	centre.y += pandist;

    setNewView( centre, sz );
}


void uiFlatViewStdControl::flipCB( CallBacker* )
{
    flip( true );
}


void uiFlatViewStdControl::parsCB( CallBacker* )
{
    doPropertiesDialog();
}


void uiFlatViewStdControl::stateCB( CallBacker* )
{
    if ( !manipdrawbut_ ) return;
    manip_ = !manip_;

    manipdrawbut_->setPixmap( manip_ ? "altview.png" : "altpick.png" );
    vwr_.rgbCanvas().setDragMode( !manip_ ? uiGraphicsViewBase::RubberBandDrag
	   				  : uiGraphicsViewBase::ScrollHandDrag);
    vwr_.rgbCanvas().scene().setMouseEventActive( true );
    if ( editbut_ )
	editbut_->setOn( false );
}


void uiFlatViewStdControl::editCB( CallBacker* )
{
    uiGraphicsViewBase::ODDragMode mode;
    if ( editbut_->isOn() )
	mode = uiGraphicsViewBase::NoDrag;
    else
	mode = manip_ ? uiGraphicsViewBase::ScrollHandDrag
	    	      : uiGraphicsViewBase::RubberBandDrag;

    vwr_.rgbCanvas().setDragMode( mode );
    vwr_.rgbCanvas().scene().setMouseEventActive( true );
}


void uiFlatViewStdControl::helpCB( CallBacker* )
{
    uiMainWin::provideHelp( helpid_ );
}


bool uiFlatViewStdControl::handleUserClick()
{
    //TODO and what about multiple viewers?
    const MouseEvent& ev = mouseEventHandler(0).event();
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	  !ev.altStatus() )
    {
	menu_.executeMenu(0);
	return true;
    }
    return false;
}


void uiFlatViewStdControl::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;

    mAddMenuItem( menu, &propertiesmnuitem_, true, false );
}


void uiFlatViewStdControl::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    bool ishandled = true;
    if ( mnuid==propertiesmnuitem_.id )
	doPropertiesDialog();
    else
	ishandled = false;

    menu->setIsHandled( ishandled );
}


void uiFlatViewStdControl::coltabChg( CallBacker* )
{
    vwr_.handleChange( FlatView::Viewer::VDPars, false );
    vwr_.handleChange( FlatView::Viewer::VDData );
}


void uiFlatViewStdControl::keyPressCB( CallBacker* )
{
    const KeyboardEvent& ev = 
	vwr_.rgbCanvas().getKeyboardEventHandler().event();
    if ( ev.key_ == OD::Escape )
	stateCB( 0 );
}
