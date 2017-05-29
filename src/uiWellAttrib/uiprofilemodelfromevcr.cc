/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		December 2016
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprofilemodelfromevcr.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uidlggroup.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewpropdlg.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiprofileviewpars.h"
#include "uistratmultidisplaywindow.h"
#include "uistratlayermodel.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uitaskrunner.h"

#include "flatposdata.h"
#include "seisdatapack.h"
#include "profilebase.h"
#include "profilemodelcreator.h"
#include "profileposprovider.h"
#include "profilemodelfromeventdata.h"
#include "profilemodelbaseauxdatamgr.h"
#include "zvalueprovider.h"

    static int sNewMarkerNameColIdx = 2;
    static int sNewMarkerColorColIdx = 3;

class uiEventMarkerTieDialog : public uiDialog
{ mODTextTranslationClass(uiEventMarkerTieDialog)
public:


uiEventMarkerTieDialog( uiParent* p, ProfileModelFromEventData& data )
    : uiDialog(p,Setup(mJoinUiStrs(sHorizon(),sSelection().toLower()),
		tr("You can use events to shape the model between the wells"),
		mTODOHelpKey))
    , data_(data)
{
    uiTable::Setup tblsu( data_.nrEvents(), 4 );
    tblsu.defrowlbl( false ).defcollbl( false );
    evmarkertietbl_ = new uiTable( this, tblsu, "EventMarkerTie" );
    BufferStringSet collabels;
    collabels.add( "Event name" ).add( "Marker to tie" )
	     .add( "New marker name" ).add( "Marker color" );
    evmarkertietbl_->setColumnLabels( collabels );

    Well::MarkerSet wms( data_.model_.get(0 )->markers_ );
    for ( int idx=1; idx<data_.model_.size(); idx++ )
    {
	const ProfileBase* prof = data_.model_.get( idx );
	if ( prof->isWell() )
	    wms.mergeOtherWell( prof->markers_ );
    }

    BufferStringSet markernms;
    wms.getNames( markernms );
    for ( int iev=0; iev<data_.nrEvents(); iev++ )
    {
	if ( !data_.isIntersectMarker(iev) )
	    continue;

	const int tiemarkeridx = markernms.indexOf( data_.getMarkerName(iev) );
	if ( tiemarkeridx>=0 )
	    markernms.removeSingle( tiemarkeridx );
    }

    for ( int evidx=0; evidx<data_.nrEvents(); evidx++ )
    {
	const ProfileModelFromEventData::Event& ev = *data_.events_[evidx];
	evmarkertietbl_->setText( RowCol(evidx,0),
				  ev.zvalprov_->getName() );
	evmarkertietbl_->setCellReadOnly( RowCol(evidx,0), true );
	uiComboBox* mrksel = new uiComboBox( 0, "marker tie" );
	mrksel->addItem( tr(ProfileModelFromEventData::addMarkerStr()) );
	mrksel->addItems( markernms );
	evmarkertietbl_->setCellObject( RowCol(evidx,1), mrksel );
	setMarkerInTable( evidx );

	mAttachCB( mrksel->selectionChanged,
		   uiEventMarkerTieDialog::markerSelChgCB );
	mrksel->setCurrentItem( ev.tiemarkernm_ );
    }
}

void setMarkerInTable( int evidx )
{
    if ( evidx<0 || evidx>=data_.nrEvents() )
    {
	pErrMsg( "Event idx is not valid" );
	return;
    }

    const Well::Marker* newmarker = data_.getIntersectMarker( evidx );
    if ( !newmarker )
    {
	uiObject* cellobj = evmarkertietbl_->getCellObject( RowCol(evidx,1) );
	mDynamicCastGet(uiComboBox*,cb,cellobj)
	if ( cb )
	    cb->setCurrentItem( data_.getMarkerName(evidx) );
    }

    RowCol newmarkerrc( evidx, sNewMarkerNameColIdx );
    if ( newmarker )
	evmarkertietbl_->setText( newmarkerrc, newmarker->name() );
    newmarkerrc.col() = sNewMarkerColorColIdx;
    uiColorInput::Setup colselsu( newmarker ? newmarker->color()
	    				    : Color::NoColor() );
    colselsu.dlgtitle( tr("Select marker color") );
    uiColorInput* colsel  = new uiColorInput( 0, colselsu );
    colsel->setSensitive( newmarker );
    evmarkertietbl_->setCellGroup( newmarkerrc, colsel );
}


void markerSelChgCB( CallBacker* cb )
{
    mDynamicCastGet(uiComboBox*,uicb,cb);
    if ( !uicb )
	return;

    RowCol selrc = evmarkertietbl_->getCell( uicb );
    BufferString selmarkernm( uicb->text() );
    const bool addmarker =
	selmarkernm==ProfileModelFromEventData::addMarkerStr();
    const int evidx = selrc.row();
    RowCol newmarkerrc( evidx, sNewMarkerNameColIdx );
    evmarkertietbl_->setCellReadOnly( newmarkerrc, !addmarker );
    if ( addmarker )
	evmarkertietbl_->setText( newmarkerrc,
				  data_.events_[evidx]->zvalprov_->getName() );
    newmarkerrc.col() = sNewMarkerColorColIdx;
    uiGroup* colgrp = evmarkertietbl_->getCellGroup( newmarkerrc );
    colgrp->setSensitive( addmarker );
    if ( addmarker )
    {
	mDynamicCastGet(uiColorInput*,colsel,colgrp);
	colsel->setColor( data_.events_[evidx]->zvalprov_->drawColor() );
    }
}


bool acceptOK( CallBacker* )
{
    if ( !evmarkertietbl_->nrRows() )
	return false;

    for ( int evidx=0; evidx<data_.nrEvents(); evidx++ )
    {
	uiObject* uiobj = evmarkertietbl_->getCellObject( RowCol(evidx,1) );
	mDynamicCastGet(uiComboBox*,mrkrsel,uiobj);
	if ( !mrkrsel )
	    continue;

	data_.setTieMarker( evidx, mrkrsel->text() );
	if ( data_.isIntersectMarker(evidx) )
	{
	    RowCol newmarkerrc( evidx, sNewMarkerNameColIdx );
	    Well::Marker* newmarker = data_.events_[evidx]->newintersectmarker_;
	    newmarker->setName( evmarkertietbl_->text(newmarkerrc) );
	    newmarkerrc.col() = sNewMarkerColorColIdx;
	    uiGroup* cellgrp = evmarkertietbl_->getCellGroup( newmarkerrc );
	    mDynamicCastGet(uiColorInput*,colsel,cellgrp);
	    if ( colsel )
	    {
		Strat::Level* newmarkerlvl =
		    Strat::eLVLS().get( data_.events_[evidx]->levelid_ );
		if ( newmarkerlvl )
		{
		    newmarkerlvl->setColor( colsel->color() );
		    newmarker->setColor( colsel->color() );
		}
	    }
	}
    }

    return true;
}

    uiTable*				evmarkertietbl_;
    ProfileModelFromEventData&		data_;
};

void uiProfileModelFromEvCrGrpFactory::addCreateFunc( CreateFunc crfn ,
						      const char* key )
{
    const int keyidx = keys_.indexOf( key );
    if ( keyidx >= 0 )
    {
	createfuncs_[keyidx] = crfn;
	return;
    }

    createfuncs_ += crfn;
    keys_.add( key );
}


uiProfileModelFromEvCrGrp* uiProfileModelFromEvCrGrpFactory::create(
	const char* key, uiParent* p, ProfileModelFromEventData& data )
{
    const int keyidx = keys_.indexOf( key );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( p, data );
}


uiProfileModelFromEvCrGrpFactory& uiPMCrGrpFac()
{
    mDefineStaticLocalObject(uiProfileModelFromEvCrGrpFactory,profmodcrfac_, );
    return profmodcrfac_;
}


class uiProfileModelViewPropDlg : public uiFlatViewPropDlg
{ mODTextTranslationClass(uiProfileModelViewPropDlg);
public:

uiProfileModelViewPropDlg( uiParent*p, FlatView::Viewer& vwr,
			   const CallBack& cb, ProfileViewPars& viewpars )
    : uiFlatViewPropDlg(p,vwr,cb)
{
    profparsgrp_ = new uiProfileViewParsDlgGrp( tabParent(), viewpars );
    addGroup( profparsgrp_ );
}

protected :

bool acceptOK( CallBacker* cb )
{
    if ( !uiFlatViewPropDlg::acceptOK(cb) )
	return false;

    profparsgrp_->readFromScreen();
    return true;
}

    uiProfileViewParsDlgGrp*	profparsgrp_;
};


class uiProfileModelViewControl : public uiFlatViewStdControl
{
public:

uiProfileModelViewControl( uiFlatViewer& vwr,
			   const uiFlatViewStdControl::Setup& su,
			   ProfileModelBaseAuxDataMgr& auxmgr )
    : uiFlatViewStdControl(vwr,su)
    , auxdatamgr_(auxmgr)
    , propdlg_(0)
{
}

virtual void doPropertiesDialog( int vieweridx )
{
    const CallBack applycb =mCB(this,uiProfileModelViewControl,applyProperties);
    if ( !propdlg_ )
	propdlg_ = new uiProfileModelViewPropDlg( setup_.parent_, vwr_, applycb,
						  auxdatamgr_.drawPars() );
    propdlg_->go();
}

virtual void applyProperties( CallBacker* cb )
{
    uiFlatViewControl::applyProperties( cb );
    auxdatamgr_.reset();
}

protected:

    ProfileModelBaseAuxDataMgr& auxdatamgr_;
    uiProfileModelViewPropDlg*	propdlg_;

};


uiProfileModelFromEvCrGrp::uiProfileModelFromEvCrGrp(
	uiParent* p, ProfileModelFromEventData& sudata )
    : uiGroup(p)
    , data_(sudata)
{
    paramgrp_ = new uiGroup( this, "Param Group" );
    nrprofsfld_ = new uiGenInput(paramgrp_,tr("Ctrl Profiles"),IntInpSpec(50));
    nrmodelsfld_ = new uiGenInput(paramgrp_,tr("Nr Models"),IntInpSpec(25));
    nrmodelsfld_->attach( leftAlignedBelow, nrprofsfld_ );
    evlistbox_ = new uiListBox( paramgrp_, "Horizon List" );
    evlistbox_->attach( leftAlignedBelow, nrmodelsfld_ );
    addevbut_ =
	new uiToolButton( paramgrp_, "plus", tr("Add event"),
			  mCB(this,uiProfileModelFromEvCrGrp,addEventCB) );
    addevbut_->attach( alignedBelow, evlistbox_ );
    rmevbut_ =
	new uiToolButton( paramgrp_, "remove", tr("Remove event"),
			  mCB(this,uiProfileModelFromEvCrGrp,removeEventCB));
    rmevbut_->attach( rightTo, addevbut_ );
    tiemarkerbut_ =
	new uiToolButton( paramgrp_, "tieevmarker",tr("Tie event to markers.."),
			  mCB(this,uiProfileModelFromEvCrGrp,tieEventsCB) );
    tiemarkerbut_->attach( rightTo, rmevbut_ );
    uiToolButton* profupdbut =
	uiToolButton::getStd( paramgrp_, OD::Apply, mCB(this,
			      uiProfileModelFromEvCrGrp,updateProfileCB),
			      tr("Update profile") );
    profupdbut->attach( rightTo, tiemarkerbut_ );
    uiGroup* dispgrp = new uiGroup( this, "Display Group" );
    viewer_ = new uiFlatViewer( dispgrp );
    viewer_->setInitialSize( uiSize(800,300) );
    viewer_->setStretch( 2, 2 );
    dispgrp->attach( rightTo, paramgrp_ );

    FlatView::Appearance& app = viewer_->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.ddpars_.wva_.allowuserchange_ = false;
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
    app.annot_.x1_.showannot_ = true;
    app.annot_.x1_.showgridlines_ = false;
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.showannot_ = true;
    app.annot_.x2_.showgridlines_ = true;
    app.annot_.allowuserchangereversedaxis_ = false;
    const FlatDataPack& seisfdp = *data_.section_.seisfdp_;
    viewer_->setPack( false, seisfdp.id() );

    modeladmgr_ = new ProfileModelBaseAuxDataMgr( data_.model_, *viewer_ );
    const StepInterval<double> dxrg = seisfdp.posData().range( true );
    Interval<float> xrg( mCast(float,dxrg.start), mCast(float,dxrg.stop) );
    const StepInterval<double> dzrg = seisfdp.posData().range( false );
    Interval<float> zrg( mCast(float,dzrg.start), mCast(float,dzrg.stop) );
    modeladmgr_->drawPars().drawctrls_ = false;
    modeladmgr_->drawPars().drawconnections_ = false;
    modeladmgr_->view2Model().setXRange( xrg );
    modeladmgr_->view2Model().setZRange( zrg );
    modeladmgr_->view2Model().setNrSeq( 25 );
    modeladmgr_->view2Model().setZInDepth( false );
    modeladmgr_->reset();

    uiFlatViewStdControl::Setup su( this );
    su.withflip( false ).isvertical( true );
    viewcontrol_ = new uiProfileModelViewControl( *viewer_, su, *modeladmgr_ );
    drawEvents();
}


uiProfileModelFromEvCrGrp::~uiProfileModelFromEvCrGrp()
{
    viewer_->removeAuxDatas( horauxdatas_ );
    deepErase( horauxdatas_ );
}


int uiProfileModelFromEvCrGrp::nrModels() const
{
    return nrmodelsfld_->getIntValue();
}


int uiProfileModelFromEvCrGrp::nrProfs() const
{
    return nrprofsfld_->getIntValue();
}


void uiProfileModelFromEvCrGrp::updateDisplay()
{
    viewer_->setViewToBoundingBox();
    updateProfileModelDisplay();
}


void uiProfileModelFromEvCrGrp::updateProfileModelDisplay()
{
    modeladmgr_->reset();
}


void uiProfileModelFromEvCrGrp::updateProfileCB( CallBacker* )
{
    updateProfileModel();
}


void uiProfileModelFromEvCrGrp::addEventCB( CallBacker* )
{
    getEvents();
    drawEvents();
    modeladmgr_->drawPars().resetMarkerNames();
}

#define mErrRet( msg, retval ) \
{ \
    uiMSG().error( msg ); \
    return retval; \
}

bool uiProfileModelFromEvCrGrp::updateProfileModel()
{
    MouseCursorChanger waitmcs( MouseCursor::Wait );
    if ( data_.model_.isEmpty() )
	mErrRet( tr("No well added to create a model from"), false )

    if ( !data_.nrEvents() )
	mErrRet( tr("No event added to create a model from"), false )
    data_.totalnrprofs_ = nrProfs();
    ProfilePosProviderFromLine* posprov =
	new ProfilePosProviderFromLine( data_.section_.linegeom_ );
    data_.sortEventsonDepthIDs();
    data_.prepareIntersectionMarkers();
    if ( !data_.warnmsg_.isEmpty() )
    {
	uiMSG().warning( data_.warnmsg_ );
	data_.warnmsg_.setEmpty();
	drawEvents();
    }

    data_.model_.regenerateWells();
    ProfileModelFromMultiEventCreator prohoruser( data_, posprov );
    uiTaskRunner uitr( this );
    if ( !prohoruser.go(&uitr) )
	return false;

    updateProfileModelDisplay();
    return true;
}


void uiProfileModelFromEvCrGrp::removeEventCB( CallBacker* )
{
    const int rmidx = evlistbox_->firstChosen();
    if ( rmidx<0 )
	return;

    data_.removeEvent( rmidx );
    evlistbox_->removeItem( rmidx );
    drawEvents();
}


void uiProfileModelFromEvCrGrp::tieEventsCB( CallBacker* )
{
    uiEventMarkerTieDialog tieevmrkrdlg( this, data_ );
    if ( tieevmrkrdlg.go() )
	modeladmgr_->drawPars().resetMarkerNames();
}


void uiProfileModelFromEvCrGrp::drawEvents()
{
    viewer_->removeAuxDatas( horauxdatas_ );
    deepErase( horauxdatas_ );
    BufferStringSet hornms;
    evlistbox_->setEmpty();
    for ( int idx=0; idx<data_.nrEvents(); idx++ )
    {
	const ZValueProvider& zvalprov = *data_.events_[idx]->zvalprov_;
	const uiString evname = zvalprov.getName();
	evlistbox_->addItem( evname, zvalprov.drawColor() );
	FlatView::AuxData* horad =
	    viewer_->createAuxData( evname.getFullString() );
	horad->cansetcursor_ = false;
	viewer_->addAuxData( horad );
	horad->linestyle_.type_ = LineStyle::Solid;
	horad->linestyle_.color_ = zvalprov.drawColor();
	horad->linestyle_.width_ = zvalprov.drawWidth();
	horauxdatas_ += horad;
    }

    mDynamicCastGet(const RegularFlatDataPack*,regfdp,
		    data_.section_.seisfdp_.ptr());
    mDynamicCastGet(const RandomFlatDataPack*,randfdp,
		    data_.section_.seisfdp_.ptr());
    if ( regfdp )
    {
	const TrcKeySampling& sectiontks = regfdp->sampling().hsamp_;
	TrcKey itrtk;
	TrcKeySamplingIterator seciter( sectiontks );
	while ( seciter.next(itrtk) )
	{
	    const int curidx = mCast( int, seciter.curIdx() );
	    for ( int iev=0; iev<data_.nrEvents(); iev++ )
	    {
		FlatView::AuxData* horad = horauxdatas_[iev];
		const float z =
		    data_.events_[iev]->zvalprov_->getZValue( itrtk );
		const float dist =
		    mCast(float,regfdp->posData().position(true,curidx));
		horad->poly_ += FlatView::Point( dist, z );
	    }
	}
    }
    else if ( randfdp )
    {
	const TrcKeyPath& rdlpath = randfdp->getPath();
	for ( int itrc=0; itrc<rdlpath.size(); itrc++ )
	{
	    const TrcKey itrtk = rdlpath[itrc];
	    for ( int iev=0; iev<data_.nrEvents(); iev++ )
	    {
		FlatView::AuxData* horad = horauxdatas_[iev];
		const float z =
		    data_.events_[iev]->zvalprov_->getZValue( itrtk );
		const float dist =
		    mCast(float,randfdp->posData().position(true,itrc));
		horad->poly_ += FlatView::Point( dist, z );
	    }
	}
    }

    viewer_->handleChange( FlatView::Viewer::Auxdata );
}


uiProfileModelFromEvCrDlg::uiProfileModelFromEvCrDlg( uiParent* p,
	ProfileModelFromEventData& sudata )
    : uiDialog(p,uiDialog::Setup(tr("Use events to shape profiles"),tr(""),
				 mNoHelpKey).applybutton(true)
					    .applytext(tr("Apply in model")))
    , data_(sudata)
{
    profscrgrp_ = uiPMCrGrpFac().create( sudata.eventtypestr_, this, sudata );
    const CallBack showcb =
	mCB(this,uiProfileModelFromEvCrDlg,showMultiDisplayCB);
    viewbut_ = new uiToolButton( this, "stratmultidisp", tr("Show display"),
				 showcb );
    viewbut_->attach( rightTo, profscrgrp_ );
    viewbut_->setSensitive( false );
    mAttachCB( applyPushed, uiProfileModelFromEvCrDlg::applyCB );
    mAttachCB( afterPopup, uiProfileModelFromEvCrDlg::finaliseCB );
}


void uiProfileModelFromEvCrDlg::showMultiDisplayCB( CallBacker* )
{
    multidispwin_ =
	new uiStratMultiDisplayWin( this, data_.section_.seisfdp_->id() );
    multidispwin_->show();
}


bool uiProfileModelFromEvCrDlg::doApply()
{
    uiStratLayerModel* uislm = uiStratLayerModel::getUILayerModel();
    if ( uislm )
	uislm->setNrModels( profscrgrp_->nrModels() );

    return profscrgrp_->updateProfileModel();
}


bool uiProfileModelFromEvCrDlg::acceptOK( CallBacker* )
{
    return doApply();
}


void uiProfileModelFromEvCrDlg::applyCB( CallBacker* )
{
    doApply();
    viewbut_->setSensitive( true );
}


void uiProfileModelFromEvCrDlg::finaliseCB( CallBacker* )
{
    profscrgrp_->updateDisplay();
}
