/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		December 2016
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprofilemodelfromevcr.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uitaskrunner.h"

#include "flatposdata.h"
#include "seisdatapack.h"
#include "profilebase.h"
#include "profilemodelcreator.h"
#include "profilemodelbaseauxdatamgr.h"
#include "zvalueprovider.h"

class uiEventMarkerTieDialog : public uiDialog
{ mODTextTranslationClass(uiEventMarkerTieDialog)
public:

uiEventMarkerTieDialog( uiParent* p, ProfModelCrData& data )
    : uiDialog(p,Setup(mJoinUiStrs(sHorizon(),sSelection().toLower()),
		tr("You can use horizons to shape the model between the wells"),
		mTODOHelpKey))
    , data_(data)
{
    uiTable::Setup tblsu( data_.zvalprovs_.size(), 2 );
    evmarkertietbl_ = new uiTable( this, tblsu, "EventMarkerTie" );

    for ( int idx=data_.tiemarkernms_.size();idx<data_.zvalprovs_.size();idx++ )
	data_.tiemarkernms_.add( ProfModelCrData::dontUseStr() );

    Well::MarkerSet wms( data_.model_.get(0 )->markers_ );
    for ( int idx=1; idx<data_.model_.size(); idx++ )
    {
	const ProfileBase* prof = data_.model_.get( idx );
	if ( prof->isWell() )
	    wms.mergeOtherWell( prof->markers_ );
    }

    BufferStringSet markernms;
    wms.getNames( markernms );

    for ( int idx=0; idx<data_.zvalprovs_.size(); idx++ )
    {
	evmarkertietbl_->setText( RowCol(idx,0),
				  data_.zvalprovs_[idx]->getName() );
	uiComboBox* mrksel = new uiComboBox( 0, "marker tie" );
	mrksel->addItem( tr(ProfModelCrData::dontUseStr()) );
	mrksel->addItems( markernms );
	mrksel->setCurrentItem( data_.tiemarkernms_[idx]->buf() );
	evmarkertietbl_->setCellObject( RowCol(idx,1), mrksel );
    }
}


bool acceptOK( CallBacker* )
{
    if ( !evmarkertietbl_->nrRows() )
	return false;

    for ( int idx=0; idx<data_.zvalprovs_.size(); idx++ )
    {
	uiObject* uiobj = evmarkertietbl_->getCellObject( RowCol(idx,1) );
	mDynamicCastGet(uiComboBox*,mrkrsel,uiobj);
	if ( !mrkrsel )
	    continue;

	data_.tiemarkernms_.get( idx ) = mrkrsel->text();
    }

    return true;
}

    uiTable*					evmarkertietbl_;
    ProfModelCrData&		data_;
};

void uiProfileModelFromEvCrGrpFactory::addCreateFunc(
	CreateFunc crfn ,const char* key )
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
	const char* key, uiParent* p,
	const ProfModelCrData& data )
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



uiProfileModelFromEvCrGrp::uiProfileModelFromEvCrGrp(
	uiParent* p, const ProfModelCrData& sudata )
    : uiGroup(p)
    , data_(* new ProfModelCrData(sudata) )
{
    paramgrp_ = new uiGroup( this, "Param Group" );
    nrprofsfld_ = new uiGenInput(paramgrp_,tr("Ctrl Profiles"),IntInpSpec(50));
    evlistbox_ = new uiListBox( paramgrp_, "Horizon List" );
    evlistbox_->attach( leftAlignedBelow, nrprofsfld_ );
    addevbut_ =
	new uiToolButton( paramgrp_, "plus", tr("Add event"),
			  mCB(this,uiProfileModelFromEvCrGrp,addEventCB) );
    addevbut_->attach( alignedBelow, evlistbox_ );
    rmevbut_ =
	new uiToolButton( paramgrp_, "remove", tr("Remove event"),
			  mCB(this,uiProfileModelFromEvCrGrp,removeEventCB));
    rmevbut_->attach( rightTo, addevbut_ );
    tiemarkerbut_ =
	new uiToolButton( paramgrp_, "", tr("Tie event to markers.."),
			  mCB(this,uiProfileModelFromEvCrGrp,tieEventsCB) );
    tiemarkerbut_->attach( rightTo, rmevbut_ );
    applybut_ =
	new uiToolButton( paramgrp_, "doall", tr("Apply.."),
			  mCB(this,uiProfileModelFromEvCrGrp,createModelCB) );
    applybut_->attach( rightTo, tiemarkerbut_ );
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
    app.annot_.x1_.name_ = "Model Nr";
    app.annot_.x2_.name_ = "Depth";
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
    viewer_->setPack( false, data_.seisfdpid_ );
    modeladmgr_ = new ProfileModelBaseAuxDataMgr( data_.model_, *viewer_ );
    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    ConstDataPackRef<FlatDataPack> seisfdp = dpm.obtain( data_.seisfdpid_ );
    const StepInterval<double> dxrg = seisfdp->posData().range( true );
    Interval<float> xrg( mCast(float,dxrg.start), mCast(float,dxrg.stop) );
    const StepInterval<double> dzrg = seisfdp->posData().range( false );
    Interval<float> zrg( mCast(float,dzrg.start), mCast(float,dzrg.stop) );
    modeladmgr_->view2Model().setXRange( xrg );
    modeladmgr_->view2Model().setZRange( zrg );
    modeladmgr_->view2Model().setNrSeq( 25 );
    modeladmgr_->view2Model().setZInDepth( false );
    modeladmgr_->reset();
}


uiProfileModelFromEvCrGrp::~uiProfileModelFromEvCrGrp()
{
    viewer_->removeAuxDatas( horauxdatas_ );
    deepErase( horauxdatas_ );
}


int uiProfileModelFromEvCrGrp::nrProfs() const
{
    return nrprofsfld_->getIntValue();
}


void uiProfileModelFromEvCrGrp::updateDisplay()
{
    viewer_->setViewToBoundingBox();
    viewer_->handleChange( FlatView::Viewer::Auxdata );
}


void uiProfileModelFromEvCrGrp::addEventCB( CallBacker* )
{
    getEvents();
    drawEvents();
}

#define mErrRet( msg, retval ) \
{ \
    uiMSG().error( msg ); \
    return retval; \
}

void uiProfileModelFromEvCrGrp::createModelCB(CallBacker*)
{
    if ( data_.model_.isEmpty() )
	mErrRet( tr("No well added to create a model from"), )
    if ( data_.zvalprovs_.isEmpty() )
	mErrRet( tr("No event added to create a model from"), )
    ProfileModelFromMultiEventCreator prohoruser(
	    data_.model_, data_.zvalprovs_, data_.tiemarkernms_,
	    data_.linegeom_, nrProfs() );
    uiTaskRunner uitr( this );
    if ( !prohoruser.go(&uitr) )
	return;

    modeladmgr_->reset();
}


void uiProfileModelFromEvCrGrp::removeEventCB( CallBacker* )
{
    const int rmidx = evlistbox_->firstChosen();
    if ( rmidx<0 )
	return;

    delete data_.zvalprovs_.removeSingle( rmidx );
    evlistbox_->removeItem( rmidx );
}


void uiProfileModelFromEvCrGrp::tieEventsCB( CallBacker* )
{
    uiEventMarkerTieDialog tieevmrkrdlg( this, data_ );
    tieevmrkrdlg.go();
}


void uiProfileModelFromEvCrGrp::drawEvents()
{
    viewer_->removeAuxDatas( horauxdatas_ );
    deepErase( horauxdatas_ );
    BufferStringSet hornms;
    evlistbox_->setEmpty();
    for ( int idx=0; idx<data_.zvalprovs_.size(); idx++ )
    {
	const ZValueProvider& zvalprov = *data_.zvalprovs_[idx];
	const uiString evname = zvalprov.getName();
	evlistbox_->addItem( evname, zvalprov.drawColor() );
	FlatView::AuxData* horad =
	    viewer_->createAuxData( evname.getFullString() );
	viewer_->addAuxData( horad );
	horad->linestyle_.type_ = LineStyle::Solid;
	horad->linestyle_.color_ = zvalprov.drawColor();
	horad->linestyle_.width_ = zvalprov.drawWidth();
	horauxdatas_ += horad;
    }

    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    ConstDataPackRef<FlatDataPack> fdp = dpm.obtain( data_.seisfdpid_ );
    mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
    mDynamicCastGet(const RandomFlatDataPack*,randfdp,fdp.ptr());
    if ( regfdp )
    {
	const TrcKeySampling& sectiontks = regfdp->sampling().hsamp_;
	TrcKey itrtk;
	TrcKeySamplingIterator seciter( sectiontks );
	while ( seciter.next(itrtk) )
	{
	    od_int64 curidx = seciter.curIdx();
	    for ( int iev=0; iev<data_.zvalprovs_.size(); iev++ )
	    {
		FlatView::AuxData* horad = horauxdatas_[iev];
		const float z = data_.zvalprovs_[iev]->getZValue( itrtk );
		const float dist =
		    regfdp->posData().position( true, mCast(int,curidx) );
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
	    for ( int iev=0; iev<data_.zvalprovs_.size(); iev++ )
	    {
		FlatView::AuxData* horad = horauxdatas_[iev];
		const float z = data_.zvalprovs_[iev]->getZValue( itrtk );
		const float dist = randfdp->posData().position( true, itrc );
		horad->poly_ += FlatView::Point( dist, z );
	    }
	}
    }

    viewer_->handleChange( FlatView::Viewer::Auxdata );
}


uiProfileModelFromEvCrDlg::uiProfileModelFromEvCrDlg( uiParent* p,
	const ProfModelCrData& sudata, const char* typenm )
    : uiDialog(p,uiDialog::Setup(tr(""),tr(""),mNoHelpKey))
{
    profscrgrp_ = uiPMCrGrpFac().create( typenm, this, sudata );
    mAttachCB( afterPopup, uiProfileModelFromEvCrDlg::finaliseCB );
}


void uiProfileModelFromEvCrDlg::finaliseCB( CallBacker* )
{
    profscrgrp_->updateDisplay();
}
