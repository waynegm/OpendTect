/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		February 2017
________________________________________________________________________

-*/

#include "uiprofilesynthseiscorrwin.h"
#include "uiprofileviewpars.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uiflatviewproptabs.h"
#include "uimultiflatviewcontrol.h"
#include "uisplitter.h"
#include "uistratlayermodel.h"
#include "uistratlaymoddisp.h"
#include "uistratsynthdisp.h"
#include "uitoolbar.h"

#include "flatposdata.h"
#include "profilemodelbaseauxdatamgr.h"
#include "profilemodelfromeventdata.h"
#include "syntheticdataimpl.h"
#include "seisbufadapters.h"


class uiProfileSynthSeisCorrViewPropDlg : public uiFlatViewPropDlg
{ mODTextTranslationClass(uiProfileSynthSeisCorrViewPropDlg);
public:

uiProfileSynthSeisCorrViewPropDlg( uiParent* p, ObjectSet<uiFlatViewer>& vwrs,
	const CallBack& cb, ProfileViewPars& viewpars )
    : uiFlatViewPropDlg(p,*vwrs[0],cb)
{
    seisproptab_  = new uiFVVDPropTab( tabParent(), *vwrs[0] );
    seisproptab_->setCaption( tr("Seismic") );
    addGroup( seisproptab_ );
    
    synthproptab_  = new uiFVVDPropTab( tabParent(), *vwrs[1] );
    synthproptab_->setCaption( tr("Synthetic") );
    addGroup( synthproptab_ );

    lmproptab_  = new uiFVVDPropTab( tabParent(), *vwrs[2] );
    lmproptab_->setCaption( tr("LayerModel") );
    addGroup( lmproptab_ );

    profparsgrp_ = new uiProfileViewParsDlgGrp( tabParent(), viewpars );
    addGroup( profparsgrp_ );
}


protected : 
    uiFVVDPropTab*		seisproptab_;
    uiFVVDPropTab*		synthproptab_;
    uiFVVDPropTab*		lmproptab_;
    uiProfileViewParsDlgGrp* 	profparsgrp_;

};


class uiProfileSynthSeisCorrControl : public uiMultiFlatViewControl
{
public:

uiProfileSynthSeisCorrControl( uiFlatViewer& vwr,
    const uiFlatViewStdControl::Setup& su,
    const ObjectSet<ProfileModelBaseAuxDataMgr> auxdatamgrs )
    : uiMultiFlatViewControl(vwr,su)
    , auxdatamgrs_(auxdatamgrs)
    , propdlg_(0)
{
}

virtual void vwrAdded( CallBacker* )
{
    uiMultiFlatViewControl::vwrAdded( 0 );
    const int ivwr = vwrs_.size()-1;
    if ( ivwr )
	toolbars_[ivwr]->display( false );
}

virtual void doPropertiesDialog( int vieweridx )
{
    const CallBack cb =mCB(this,uiProfileSynthSeisCorrControl,applyProperties);
    if ( !propdlg_ )
	propdlg_ = new uiProfileSynthSeisCorrViewPropDlg(
		setup_.parent_, vwrs_, cb, auxdatamgrs_[0]->drawPars() );
    propdlg_->go();
}

virtual void applyProperties( CallBacker* )
{
    for ( int ivwr=0; ivwr<vwrs_.size(); ivwr++ )
    {
	vwrs_[ivwr]->handleChange( FlatView::Viewer::Annot |
				   FlatView::Viewer::DisplayPars );
	vwrs_[ivwr]->dispPropChanged.trigger();
    }

    for ( int ivwr=0; ivwr<vwrs_.size(); ivwr++ )
    {
	if ( ivwr )
	    auxdatamgrs_[ivwr]->drawPars() = auxdatamgrs_[0]->drawPars();
	auxdatamgrs_[ivwr]->reset();
    }
}

uiProfileSynthSeisCorrViewPropDlg* 	propdlg_;
ObjectSet<ProfileModelBaseAuxDataMgr>	auxdatamgrs_;

};


uiProfileSynthSeisCorrWin::uiProfileSynthSeisCorrWin(
	uiParent* p, const ProfileModelFromEventData& sudata, int nrseq )
    : uiMainWin(p,uiMainWin::Setup(tr("Seismic-Synthetic-Profile correlation")))
    , proffromevdata_(sudata)
    , uislm_(*uiStratLayerModel::getUILayerModel())
    , nrseq_(nrseq)
{
    topgrp_ = new uiGroup( this );
    uiGroup* seisgrp = createSeisDisplay();
    uiSplitter* spl1 = new uiSplitter( topgrp_, "Seis-Synth", false );
    uiGroup* synthgrp = createSynthDisplay();
    spl1->addGroup( seisgrp ); spl1->addGroup( synthgrp );
    uiSplitter* spl2 = new uiSplitter( this, "Seis-Synth", false );
    uiGroup* lmgrp = createLMDisplay();
    spl2->addGroup( topgrp_ ); spl2->addGroup( lmgrp );
    setSeisViewerZrg();
    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withcoltabed(false).withflip(false).withsnapshot(false);
    mDynamicCastGet(uiFlatViewer*,seisvwr,seisgrp);
    mDynamicCastGet(uiFlatViewer*,lmvwr,lmgrp);
    ObjectSet<ProfileModelBaseAuxDataMgr> auxdatamgrs;
    auxdatamgrs += seisprofadmgr_;
    auxdatamgrs += synthprofadmgr_;
    auxdatamgrs += laymodprofadmgr_;
    control_ = new uiProfileSynthSeisCorrControl( *seisvwr, fvsu, auxdatamgrs );
    if ( synthvwr_ )
	control_->addViewer( *synthvwr_ );
    if ( lmvwr )
	control_->addViewer( *lmvwr );
    control_->reInitZooms();
}


void uiProfileSynthSeisCorrWin::initViewer( uiFlatViewer* vwr )
{
    vwr->setInitialSize( uiSize(800,300) );
    vwr->setStretch( 2, 2 );

    FlatView::Appearance& app = vwr->appearance();
    app.ddpars_.show( false, true );
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
    app.annot_.allowuserchange_ = false;
}


void setProfAuxDataMgr( ProfileModelBaseAuxDataMgr& auxdatamgr,
			uiFlatViewer& vwr, int nrseq )
{
    auxdatamgr.drawPars().drawctrls_ = false;
    auxdatamgr.drawPars().drawconnections_ = false;
    DataPack::ID dpid = vwr.packID( false );
    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain(dpid);
    mDynamicCastGet(FlatDataPack*,fdp,dp);
    if ( fdp )
    {
	const StepInterval<double> dxrg = fdp->posData().range( true );
	Interval<float> xrg( mCast(float,dxrg.start), mCast(float,dxrg.stop) );
	const StepInterval<double> dzrg = fdp->posData().range( false );
	Interval<float> zrg( mCast(float,dzrg.start), mCast(float,dzrg.stop) );
	auxdatamgr.view2Model().setXRange( xrg );
	auxdatamgr.view2Model().setZRange( zrg );
	auxdatamgr.view2Model().setNrSeq( nrseq );
    }

    auxdatamgr.reset();
}


uiGroup* uiProfileSynthSeisCorrWin::createSeisDisplay()
{
    seisvwr_ = new uiFlatViewer( topgrp_ );
    initViewer( seisvwr_ );
    seisvwr_->setPack( false, proffromevdata_.section_.seisfdp_->id() );
    seisvwr_->setViewToBoundingBox();
    seisprofadmgr_ =
	new ProfileModelBaseAuxDataMgr( proffromevdata_.model_, *seisvwr_ );
    seisprofadmgr_->view2Model().setZInDepth( false );
    setProfAuxDataMgr( *seisprofadmgr_, *seisvwr_, nrseq_ );
    return seisvwr_;
}


uiGroup* uiProfileSynthSeisCorrWin::createSynthDisplay()
{
    uiGroup* synthdispgrp = uislm_.getSynthDisp()->getDisplayClone( topgrp_ );
    mDynamicCast(uiFlatViewer*,synthvwr_,synthdispgrp);
    initViewer( synthvwr_ );
    synthprofadmgr_ =
	new ProfileModelBaseAuxDataMgr( proffromevdata_.model_, *synthvwr_ );
    synthprofadmgr_->view2Model().setZInDepth( false );
    setProfAuxDataMgr( *synthprofadmgr_, *synthvwr_, nrseq_ );
    return synthdispgrp;
}


void uiProfileSynthSeisCorrWin::setSeisViewerZrg()
{
    const StepInterval<double> seisxrg = seisvwr_->posRange( true );
    const StepInterval<double> synthzrg = synthvwr_->posRange( false );
    seisvwr_->setSelDataRanges( seisxrg, synthzrg );
}


uiGroup* uiProfileSynthSeisCorrWin::createLMDisplay()
{
    uiGroup* lmdispgrp = uislm_.getLayModelDisp()->getDisplayClone( this );
    mDynamicCastGet(uiFlatViewer*,laymodvwr,lmdispgrp);
    initViewer( laymodvwr );
    laymodprofadmgr_ =
	new ProfileModelBaseAuxDataMgr( proffromevdata_.model_, *laymodvwr );
    setProfAuxDataMgr( *laymodprofadmgr_, *laymodvwr, nrseq_ );
    laymodprofadmgr_->view2Model().setZInDepth( true );
    return lmdispgrp;
}
