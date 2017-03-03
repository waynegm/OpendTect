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
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistratmultidisplaywindow.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uitaskrunner.h"

#include "flatposdata.h"
#include "seisdatapack.h"
#include "profilebase.h"
#include "profilemodelcreator.h"
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

    for ( int evidx=0; evidx<data_.nrEvents(); evidx++ )
    {
	const ProfileModelFromEventData::Event& ev = *data_.events_[evidx];
	evmarkertietbl_->setText( RowCol(evidx,0),
				  ev.zvalprov_->getName() );
	evmarkertietbl_->setCellReadOnly( RowCol(evidx,0), true );
	uiComboBox* mrksel = new uiComboBox( 0, "marker tie" );
	mrksel->addItem( tr(ProfileModelFromEventData::addMarkerStr()) );
	mrksel->addItem( tr(ProfileModelFromEventData::dontUseStr()) );
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
	return;

    RowCol newmarkerrc( evidx, sNewMarkerNameColIdx );
    evmarkertietbl_->setText( newmarkerrc, newmarker->name() );
    newmarkerrc.col() = sNewMarkerColorColIdx;
    uiColorInput::Setup colselsu( newmarker->color() );
    colselsu.dlgtitle( tr("Select marker color") );
    uiColorInput* colsel  = new uiColorInput( 0, colselsu );
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
    newmarkerrc.col() = sNewMarkerColorColIdx;
    uiGroup* colsel = evmarkertietbl_->getCellGroup( newmarkerrc );
    colsel->setSensitive( addmarker );
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
		newmarker->setColor( colsel->color() );
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


uiProfileModelFromEvCrGrp::uiProfileModelFromEvCrGrp(
	uiParent* p, ProfileModelFromEventData& sudata )
    : uiGroup(p)
    , data_(sudata)
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
	new uiToolButton( paramgrp_, "tieevmarker",tr("Tie event to markers.."),
			  mCB(this,uiProfileModelFromEvCrGrp,tieEventsCB) );
    tiemarkerbut_->attach( rightTo, rmevbut_ );
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
    viewer_->setPack( false, data_.section_.seisfdpid_ );
    modeladmgr_ = new ProfileModelBaseAuxDataMgr( data_.model_, *viewer_ );
    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    ConstDataPackRef<FlatDataPack> seisfdp =
	dpm.obtain( data_.section_.seisfdpid_ );
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
    updateProfileModelDisplay();
}


void uiProfileModelFromEvCrGrp::updateProfileModelDisplay()
{
    modeladmgr_->reset();
}


void uiProfileModelFromEvCrGrp::addEventCB( CallBacker* )
{
    getEvents();
    drawEvents();
    updateProfileModel();
}

#define mErrRet( msg, retval ) \
{ \
    uiMSG().error( msg ); \
    return retval; \
}

void uiProfileModelFromEvCrGrp::updateProfileModel()
{
    if ( data_.model_.isEmpty() )
	mErrRet( tr("No well added to create a model from"), )

    checkAndRemoveEvents();
    if ( !data_.nrEvents() )
	mErrRet( tr("No event added to create a model from"), )
    data_.totalnrprofs_ = nrProfs();
    ProfileModelFromMultiEventCreator prohoruser( data_ );
    uiTaskRunner uitr( this );
    if ( !prohoruser.go(&uitr) )
	return;

    updateProfileModelDisplay();
}


void uiProfileModelFromEvCrGrp::removeEventCB( CallBacker* )
{
    const int rmidx = evlistbox_->firstChosen();
    if ( rmidx<0 )
	return;

    data_.removeEvent( rmidx );
    evlistbox_->removeItem( rmidx );
    drawEvents();
    updateProfileModel();
}


void uiProfileModelFromEvCrGrp::tieEventsCB( CallBacker* )
{
    uiEventMarkerTieDialog tieevmrkrdlg( this, data_ );
    if ( tieevmrkrdlg.go() )
	updateProfileModel();
}


void uiProfileModelFromEvCrGrp::checkAndRemoveEvents()
{
    uiStringSet evnms;

    for ( int iev=data_.events_.size()-1; iev>=0; iev-- )
    {
	if ( !data_.isIntersectMarker(iev) )
	    continue;

	const int firstwellidx = data_.model_.nearestIndex( 0.0f, true );
	const int lastwellidx = data_.model_.nearestIndex( 1.0f, true );
	if ( firstwellidx<0 || lastwellidx<0 )
	{
	    pErrMsg( "Cannot find well" );
	    evnms += data_.events_[iev]->zvalprov_->getName();
	    evlistbox_->removeItem( iev );
	    data_.removeEvent( iev );
	    continue;
	}

	const Coord firstwellpos = data_.model_.get( firstwellidx )->coord_;
	const float firstwellintz = data_.getZValue( iev, firstwellpos );
	const Coord lastwellpos = data_.model_.get( lastwellidx )->coord_;
	const float lastwellintz = data_.getZValue( iev, lastwellpos );
	if ( mIsUdf(firstwellintz) || mIsUdf(lastwellintz) )
	{
	    evnms += data_.events_[iev]->zvalprov_->getName();
	    evlistbox_->removeItem( iev );
	    data_.removeEvent( iev );
	}
    }

    if ( !evnms.isEmpty() )
	mErrRet( tr("Removing event '%1' as they do not intersect wells "
		    "at extereme positions").arg(evnms.cat(",")), )
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
	viewer_->addAuxData( horad );
	horad->linestyle_.type_ = LineStyle::Solid;
	horad->linestyle_.color_ = zvalprov.drawColor();
	horad->linestyle_.width_ = zvalprov.drawWidth();
	horauxdatas_ += horad;
    }

    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    ConstDataPackRef<FlatDataPack> fdp = dpm.obtain( data_.section_.seisfdpid_);
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
	    for ( int iev=0; iev<data_.nrEvents(); iev++ )
	    {
		FlatView::AuxData* horad = horauxdatas_[iev];
		const float z =
		    data_.events_[iev]->zvalprov_->getZValue( itrtk );
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
	    for ( int iev=0; iev<data_.nrEvents(); iev++ )
	    {
		FlatView::AuxData* horad = horauxdatas_[iev];
		const float z =
		    data_.events_[iev]->zvalprov_->getZValue( itrtk );
		const float dist = randfdp->posData().position( true, itrc );
		horad->poly_ += FlatView::Point( dist, z );
	    }
	}
    }

    viewer_->handleChange( FlatView::Viewer::Auxdata );
}


uiProfileModelFromEvCrDlg::uiProfileModelFromEvCrDlg( uiParent* p,
	ProfileModelFromEventData& sudata, const char* typenm )
    : uiDialog(p,uiDialog::Setup(tr("Use events to shape profiles"),
				 tr("Add events that you want to use"),
				 mNoHelpKey).applybutton(true)
					    .applytext(tr("Apply in model")))
    , data_(sudata)
{
    profscrgrp_ = uiPMCrGrpFac().create( typenm, this, sudata );
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
	new uiStratMultiDisplayWin( this, data_.section_.seisfdpid_ );
    multidispwin_->show();
}


void uiProfileModelFromEvCrDlg::applyCB( CallBacker* )
{
    viewbut_->setSensitive( true );
}


void uiProfileModelFromEvCrDlg::finaliseCB( CallBacker* )
{
    profscrgrp_->updateDisplay();
}
