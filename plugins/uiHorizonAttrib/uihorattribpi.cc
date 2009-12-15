/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorattribpi.cc,v 1.21 2009-12-15 10:48:17 cvsraman Exp $";

#include "uihorizonattrib.h"
#include "uicontourtreeitem.h"
#include "uiempartserv.h"
#include "uistratamp.h"
#include "uiflattenedcube.h"
#include "uiisopachmaker.h"
#include "uicalcpoly2horvol.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodemsurftreeitem.h"
#include "vishorizondisplay.h"
#include "vispicksetdisplay.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "uipickpartserv.h"
#include "attribsel.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "plugins.h"


static const char* sKeyContours = "Contours";

mExternC int GetuiHorizonAttribPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiHorizonAttribPluginInfo()
{
    static PluginInfo retpi = {
	"Horizon-Attribute",
	"dGB - Nanne Hemstra",
	"=od",
	"The 'Horizon' Attribute allows getting values from horizons.\n"
    	"Not to be confused with calculating attributes on horizons.\n"
        "It can even be useful to apply the 'Horizon' attribute on horizons.\n"
        "Also, the Stratal Amplitude is provided by this plugin,\n"
	"and the writing of flattened cubes" };
    return &retpi;
}


class uiHorAttribPIMgr :  public CallBacker
{
public:
			uiHorAttribPIMgr(uiODMain*);

    void		updateMenu(CallBacker*);
    void		makeStratAmp(CallBacker*);
    void		doFlattened(CallBacker*);
    void		doIsopach(CallBacker*);
    void		doIsopachThruMenu(CallBacker*);
    void		doContours(CallBacker*);
    void		calcPolyVol(CallBacker*);
    void		calcHorVol(CallBacker*);

    uiODMain*		appl_;
    uiVisMenuItemHandler flattenmnuitemhndlr_;
    uiVisMenuItemHandler isopachmnuitemhndlr_;
    uiVisMenuItemHandler contourmnuitemhndlr_;
    uiVisMenuItemHandler horvolmnuitemhndlr_;
    uiVisMenuItemHandler polyvolmnuitemhndlr_;
};


#define mMkPars(txt,fun) \
    visSurvey::HorizonDisplay::getStaticClassName(), \
    *a->applMgr().visServer(),txt,mCB(this,uiHorAttribPIMgr,fun)

uiHorAttribPIMgr::uiHorAttribPIMgr( uiODMain* a )
	: appl_(a)
    	, flattenmnuitemhndlr_(mMkPars("Write &Flattened cube ...",doFlattened))
    	, isopachmnuitemhndlr_(mMkPars("Calculate &Isopach ...",doIsopach))
	, contourmnuitemhndlr_(mMkPars("Add &Contour Display",doContours),995)
    	, horvolmnuitemhndlr_(mMkPars("Calculate &Volume ...",calcHorVol))
	, polyvolmnuitemhndlr_(visSurvey::PickSetDisplay::getStaticClassName(),
		*a->applMgr().visServer(),"Calculate &Volume ...",
		mCB(this,uiHorAttribPIMgr,calcPolyVol),996)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mnumgr.dTectMnuChanged.notify(mCB(this,uiHorAttribPIMgr,updateMenu));
    updateMenu(0);
}


void uiHorAttribPIMgr::updateMenu( CallBacker* )
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    MenuItemSeparString gridprocstr( "Create Grid output" );
    uiMenuItem* itm = mnumgr.procMnu()->find( gridprocstr );
    if ( !itm ) return;

    mDynamicCastGet(uiPopupItem*,gridpocitm,itm)
    if ( !gridpocitm ) return;

    gridpocitm->menu().insertItem( new uiMenuItem("&Stratal Amplitude ...",
	       			  mCB(this,uiHorAttribPIMgr,makeStratAmp)) );

    gridpocitm->menu().insertItem( new uiMenuItem("&Isopach ...", mCB(this,
		    			 uiHorAttribPIMgr,doIsopachThruMenu)) );
}


void uiHorAttribPIMgr::makeStratAmp( CallBacker* )
{
    uiStratAmpCalc dlg( appl_ );
    dlg.go();
}


void uiHorAttribPIMgr::doFlattened( CallBacker* )
{
    const int displayid = flattenmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    uiWriteFlattenedCube dlg( appl_, hd->getObjectID() );
    dlg.go();
}


void uiHorAttribPIMgr::doIsopach( CallBacker* )
{
    const int displayid = isopachmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    if ( !visserv->canAddAttrib(displayid) )
    {
	uiMSG().error( "Cannot add extra attribute layers" );
	return;
    }

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;
    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
    if ( !parent ) return;

    uiIsopachMaker dlg( appl_, hd->getObjectID() );
    if ( !dlg.go() )
	return;

    const int attrid = visserv->addAttrib( displayid );
    Attrib::SelSpec selspec( dlg.attrName(), Attrib::SelSpec::cOtherAttrib(),
	    		     false, 0 );
    visserv->setSelSpec( displayid, attrid, selspec );
    visserv->createAndDispDataPack( displayid, attrid, &dlg.getDPS() );
    uiODAttribTreeItem* itm = new uiODEarthModelSurfaceDataTreeItem(
	    	hd->getObjectID(), 0, typeid(*parent).name() );
    parent->addChild( itm, false );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
    parent->updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiHorAttribPIMgr::doIsopachThruMenu( CallBacker* )
{
    uiIsopachMaker dlg( appl_, -1 );
    if ( !dlg.go() )
	return;
}


void uiHorAttribPIMgr::doContours( CallBacker* cb )
{
    const int displayid = contourmnuitemhndlr_.getDisplayID();
    uiTreeItem* parent = appl_->sceneMgr().findItem( displayid );
    if ( !parent )
	return;

    const uiTreeItem* item = parent->findChild( sKeyContours );
    if ( item )
    {
	mDynamicCastGet(const uiContourTreeItem*,conitm,item);
	if ( conitm )
	    return;
    }

    uiVisPartServer* visserv = appl_->applMgr().visServer();
    if ( !visserv->canAddAttrib(displayid) )
    {
	uiMSG().error( "Cannot add extra attribute layers" );
	return;
    }

    const int attrib = visserv->addAttrib( displayid );
    Attrib::SelSpec spec( sKeyContours, Attrib::SelSpec::cAttribNotSel(),
	    		  false, 0 );
    spec.setDefString( uiContourTreeItem::sKeyContourDefString() );
    visserv->setSelSpec( displayid, attrib, spec );

    uiContourTreeItem* newitem = new uiContourTreeItem(typeid(*parent).name());
    parent->addChild( newitem, false );
    parent->updateColumnText( uiODSceneMgr::cNameColumn() );
}


void uiHorAttribPIMgr::calcPolyVol( CallBacker* )
{
    const int displayid = polyvolmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
	    	    visserv->getObject(displayid))
    if ( !psd || !psd->getSet() )
	{ pErrMsg("Can't get PickSetDisplay"); return; }

    uiCalcPolyHorVol dlg( appl_, *psd->getSet() );
    dlg.go();
}


void uiHorAttribPIMgr::calcHorVol( CallBacker* )
{
    const int displayid = horvolmnuitemhndlr_.getDisplayID();
    uiVisPartServer* visserv = appl_->applMgr().visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,visserv->getObject(displayid))
    if ( !hd ) return;

    EM::EMObject* emobj = EM::EMM().getObject( hd->getObjectID() );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor ) { uiMSG().error("Internal: cannot find horizon"); return; }
    uiCalcHorPolyVol dlg( appl_, *hor );
    dlg.go();
}


mExternC const char* InituiHorizonAttribPlugin( int, char** )
{
    uiHorizonAttrib::initClass();
    uiContourTreeItem::initClass();
    static uiHorAttribPIMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiHorAttribPIMgr( ODMainWin() );

    return 0;
}
