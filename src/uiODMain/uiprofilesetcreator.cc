/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		December 2016
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprofilesetcreator.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uitaskrunner.h"

#include "flatposdata.h"
#include "seisdatapack.h"
#include "profilebase.h"
#include "profilesetcreator.h"
#include "zvalueprovider.h"

class uiEventMarkerTieDialog : public uiDialog
{ mODTextTranslationClass(uiEventMarkerTieDialog)
public:

uiEventMarkerTieDialog( uiParent* p, uiProfileSetCreatorGrp::Data& data )
    : uiDialog(p,Setup(mJoinUiStrs(sHorizon(),sSelection().toLower()),
		tr("You can use horizons to shape the model between the wells"),
		mTODOHelpKey))
    , data_(data)
{
    uiTable::Setup tblsu( data_.zvalprovs_.size(), 2 );
    evmarkertietbl_ = new uiTable( this, tblsu, "EventMarkerTie" );

    for ( int idx=data_.tiemarkernms_.size();idx<data_.zvalprovs_.size();idx++ )
	data_.tiemarkernms_.add( uiProfileSetCreatorGrp::Data::dontUseStr() );

    Well::MarkerSet wms( data_.profs_.get(0 )->markers_ );
    for ( int idx=1; idx<data_.profs_.size(); idx++ )
    {
	const ProfileBase* prof = data_.profs_.get( idx );
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
	mrksel->addItem( tr(uiProfileSetCreatorGrp::Data::dontUseStr()) );
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
    uiProfileSetCreatorGrp::Data&		data_;
};

void uiProfileSetCreatorGrpFactory::addCreateFunc(
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


uiProfileSetCreatorGrp* uiProfileSetCreatorGrpFactory::create(
	const char* key, uiParent* p,
	const uiProfileSetCreatorGrp::Data& data )
{
    const int keyidx = keys_.indexOf( key );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( p, data );
}


uiProfileSetCreatorGrpFactory& uiProfSetCRFac()
{
    mDefineStaticLocalObject(uiProfileSetCreatorGrpFactory,pwmsfac_, );
    return pwmsfac_;
}



uiProfileSetCreatorGrp::uiProfileSetCreatorGrp(
	uiParent* p, const uiProfileSetCreatorGrp::Data& sudata )
    : uiGroup(p)
    , data_(* new uiProfileSetCreatorGrp::Data(sudata) )
{
    paramgrp_ = new uiGroup( this, "Param Group" );
    nrprofsfld_ = new uiGenInput(paramgrp_,tr("Ctrl Profiles"),IntInpSpec(50));
    evlistbox_ = new uiListBox( paramgrp_, "Horizon List" );
    evlistbox_->attach( leftAlignedBelow, nrprofsfld_ );
    addevbut_ =
	new uiToolButton( paramgrp_, "plus", tr("Add event"),
			  mCB(this,uiProfileSetCreatorGrp,addEventCB) );
    addevbut_->attach( alignedBelow, evlistbox_ );
    rmevbut_ =
	new uiToolButton( paramgrp_, "remove", tr("Remove event"),
			  mCB(this,uiProfileSetCreatorGrp,removeEventCB));
    rmevbut_->attach( rightTo, addevbut_ );
    tiemarkerbut_ =
	new uiToolButton( paramgrp_, "", tr("Tie event to markers.."),
			  mCB(this,uiProfileSetCreatorGrp,tieEventsCB) );
    tiemarkerbut_->attach( rightTo, rmevbut_ );
    applybut_ =
	new uiToolButton( paramgrp_, "doall", tr("Apply.."),
			  mCB(this,uiProfileSetCreatorGrp,createProfileSetCB) );
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
}


uiProfileSetCreatorGrp::~uiProfileSetCreatorGrp()
{
    viewer_->removeAuxDatas( horauxdatas_ );
    deepErase( horauxdatas_ );
}


int uiProfileSetCreatorGrp::nrProfs() const
{
    return nrprofsfld_->getIntValue();
}


void uiProfileSetCreatorGrp::updateDisplay()
{
    viewer_->setViewToBoundingBox();
    viewer_->handleChange( FlatView::Viewer::Auxdata );
}


void uiProfileSetCreatorGrp::addEventCB( CallBacker* )
{
    getEvents();
    drawEvents();
}


void uiProfileSetCreatorGrp::createProfileSetCB(CallBacker*)
{
    ProfileSetFromMultiEventCreator prohoruser(
	    data_.profs_, data_.zvalprovs_, data_.tiemarkernms_,
	    data_.linegeom_, nrProfs() );
    uiTaskRunner uitr( this );
    prohoruser.go( &uitr );
}


void uiProfileSetCreatorGrp::removeEventCB( CallBacker* )
{
    const int rmidx = evlistbox_->firstChosen();
    if ( rmidx<0 )
	return;

    delete data_.zvalprovs_.removeSingle( rmidx );
    evlistbox_->removeItem( rmidx );
}


void uiProfileSetCreatorGrp::tieEventsCB( CallBacker* )
{
    uiEventMarkerTieDialog tieevmrkrdlg( this, data_ );
    tieevmrkrdlg.go();
}


void uiProfileSetCreatorGrp::drawEvents()
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
