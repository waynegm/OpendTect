/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		February 2017
________________________________________________________________________

-*/

#include "uistratmultidisplaywindow.h"
#include "uiflatviewer.h"
#include "uimultiflatviewcontrol.h"
#include "uisplitter.h"
#include "uistratlayermodel.h"
#include "uistratlaymoddisp.h"
#include "uistratsynthdisp.h"

#include "syntheticdataimpl.h"
#include "seisbufadapters.h"


uiStratMultiDisplayWin::uiStratMultiDisplayWin( uiParent* p, DataPack::ID dpid )
    : uiMainWin(p)
    , uislm_(*uiStratLayerModel::getUILayerModel())
{
    topgrp_ = new uiGroup( this );
    uiGroup* seisgrp = createSeisDisplay( dpid );
    uiSplitter* spl1 = new uiSplitter( topgrp_, "Seis-Synth", false );
    uiGroup* synthgrp = createSynthDisplay();
    spl1->addGroup( seisgrp ); spl1->addGroup( synthgrp );
    uiSplitter* spl2 = new uiSplitter( this, "Seis-Synth", false );
    uiGroup* lmgrp = createLMDisplay();
    spl2->addGroup( topgrp_ ); spl2->addGroup( lmgrp );
    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withcoltabed(false).withflip(false).withsnapshot(false);
    mDynamicCastGet(uiFlatViewer*,seisvwr,seisgrp);
    mDynamicCastGet(uiFlatViewer*,synthvwr,synthgrp);
    mDynamicCastGet(uiFlatViewer*,lmvwr,lmgrp);
    control_ = new uiMultiFlatViewControl( *seisvwr, fvsu );
    if ( synthvwr )
	control_->addViewer( *synthvwr );
    if ( lmvwr )
	control_->addViewer( *lmvwr );
    control_->reInitZooms();
}


void uiStratMultiDisplayWin::initViewer( uiFlatViewer* vwr )
{
    vwr->setInitialSize( uiSize(800,300) );
    vwr->setStretch( 2, 2 );

    FlatView::Appearance& app = vwr->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.ddpars_.wva_.allowuserchange_ = false;
    app.ddpars_.vd_.allowuserchange_ = false;
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
    app.annot_.x1_.showannot_ = true;
    app.annot_.x1_.showgridlines_ = false;
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.showannot_ = true;
    app.annot_.x2_.showgridlines_ = true;
    app.annot_.allowuserchangereversedaxis_ = false;
}


uiGroup* uiStratMultiDisplayWin::createSeisDisplay( DataPack::ID dpid )
{
    seisvwr_ = new uiFlatViewer( topgrp_ );
    initViewer( seisvwr_ );
    seisvwr_->setPack( false, dpid );
    seisvwr_->setViewToBoundingBox();
    return seisvwr_;
}


uiGroup* uiStratMultiDisplayWin::createSynthDisplay()
{
    return uislm_.getSynthDisp()->getDisplayClone( topgrp_ );
}

uiGroup* uiStratMultiDisplayWin::createLMDisplay()
{
    return uislm_.getLayModelDisp()->getDisplayClone( this );
}
