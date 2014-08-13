/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"

#include "uiaxishandler.h"
#include "uicombobox.h"
#include "uihistogramdisplay.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"

#include "arraynd.h"
#include "draw.h"
#include "bufstring.h"
#include "datapackbase.h"
#include "datapointset.h"
#include "mouseevent.h"
#include "statparallelcalc.h"

#define mPutCountInPlot() (histgramdisp_ && setup_.countinplot_)


uiStatsDisplay::uiStatsDisplay( uiParent* p, const uiStatsDisplay::Setup& su )
    : uiGroup( p, "Statistics display group" )
    , setup_(su)
    , histgramdisp_(0)
    , minmaxfld_(0)
    , countfld_(0)
    , namefld_(0)
{
    const bool putcountinplot = mPutCountInPlot();
    if ( setup_.withplot_ )
    {
	uiHistogramDisplay::Setup fsu;
	fsu.annoty( setup_.vertaxis_ );
	histgramdisp_ = new uiHistogramDisplay( this, fsu );
    }

    uiSeparator* sep = 0;
    if ( setup_.withplot_ && setup_.withtext_ )
    {
	sep = new uiSeparator( this, "Hor sep" );
	sep->attach( stretchedBelow, histgramdisp_ );
    }

    if ( setup_.withtext_ )
    {
	if ( setup_.withname_ )
	    namefld_ = new uiLabel( this, "Data Name" );

	uiGroup* valgrp = new uiGroup( this, "Values group" );
	if ( setup_.withname_ )
	    valgrp->attach( alignedBelow, namefld_ );
	minmaxfld_ = new uiGenInput( valgrp, "Value range",
				     FloatInpSpec(), FloatInpSpec() );
	minmaxfld_->setReadOnly();
	avgstdfld_ = new uiGenInput( valgrp, "Mean/Std Deviation",
				     DoubleInpSpec(), DoubleInpSpec() );
	avgstdfld_->attach( alignedBelow, minmaxfld_ );
	avgstdfld_->setReadOnly();
	medrmsfld_ = new uiGenInput( valgrp, "Median/RMS",
				     FloatInpSpec(), DoubleInpSpec() );
	medrmsfld_->attach( alignedBelow, avgstdfld_ );
	medrmsfld_->setReadOnly();
	if ( !putcountinplot )
	{
	    countfld_ = new uiGenInput( valgrp, "Number of values" );
	    countfld_->setReadOnly();
	    countfld_->attach( alignedBelow, medrmsfld_ );
	}

	if ( sep )
	{
	    valgrp->attach( centeredBelow, histgramdisp_ );
	    valgrp->attach( ensureBelow, sep );
	}
    }

    if ( putcountinplot )
	putN();
}


void uiStatsDisplay::setDataName( const char* nm )
{
    if ( namefld_ )
    {
	namefld_->setText( nm );
	namefld_->setAlignment( Alignment::HCenter );
    }
}


bool uiStatsDisplay::setDataPackID( DataPack::ID dpid, DataPackMgr::ID dmid )
{
    TypeSet<float> valarr;
    if ( !histgramdisp_ ||
	 (histgramdisp_ && !histgramdisp_->setDataPackID(dpid,dmid)) )
    {
	Stats::ParallelCalc<float> rc( (Stats::CalcSetup()
						.require(Stats::Min)
						.require(Stats::Max)
						.require(Stats::Average)
						.require(Stats::Median)
						.require(Stats::StdDev)
						.require(Stats::RMS)) );

	DataPackMgr& dpman = DPM( dmid );
	const DataPack* datapack = dpman.obtain( dpid );
	if ( !datapack ) return false;

	if ( dmid == DataPackMgr::CubeID() )
	{
	    mDynamicCastGet(const ::CubeDataPack*,cdp,datapack);
	    const Array3D<float>* arr3d = cdp ? &cdp->data() : 0;
	    if ( !arr3d ) return false;

	    const float* array = arr3d->getData();
	    rc.setValues( array, mCast(int,arr3d->info().getTotalSz()) );
	}
	else if ( dmid == DataPackMgr::FlatID() )
	{
	    const Array2D<float>* array = 0;
	    mDynamicCastGet(const FlatDataPack*,fdp,datapack);
	    mDynamicCastGet(const MapDataPack*,mdp,datapack);
	    if ( mdp )
		array = &mdp->rawData();
	    else if ( fdp )
		array = &fdp->data();

	    if ( !array )
		return false;

	    if ( array->getData() )
		rc.setValues( array->getData(),
				mCast(int,array->info().getTotalSz()) );
	    else
	    {
		const int sz2d0 = array->info().getSize( 0 );
		const int sz2d1 = array->info().getSize( 1 );
		for ( int idx0=0; idx0<sz2d0; idx0++ )
		{
		    for ( int idx1=0; idx1<sz2d1; idx1++ )
		    {
			const float val = array->get( idx0, idx1 );
			if ( mIsUdf(val) ) continue;

			valarr += val;
		    }
		}

		rc.setValues( valarr.arr(), valarr.size() );
	    }
	}
	else if ( dmid == DataPackMgr::SurfID() )
	{
	    mDynamicCastGet(const DataPointSet*,dpset,datapack)
	    if ( !dpset )
		return false;

	    for ( int idx=0; idx<dpset->size(); idx++ )
		valarr += dpset->value( 2, idx );

	    rc.setValues( valarr.arr(), valarr.size() );
	}
	if ( !rc.execute() )
	    { uiMSG().error( rc.errMsg() ); return false; }

	setData( rc );
	return false;
    }

    setData( histgramdisp_->getStatCalc() );
    return true;
}


void uiStatsDisplay::setData( const float* array, int sz )
{
    if ( !histgramdisp_ )
	return;

    histgramdisp_->setData( array, sz );
    setData( histgramdisp_->getStatCalc() );
}


void uiStatsDisplay::setData( const Array2D<float>* array )
{
    if ( !histgramdisp_ )
	return;

    histgramdisp_->setData( array );
    setData( histgramdisp_->getStatCalc() );
}


void uiStatsDisplay::setData( const Stats::ParallelCalc<float>& rc )
{
    if ( mPutCountInPlot() )
	putN();

    if ( !minmaxfld_ ) return;

    if ( countfld_ )
	countfld_->setValue( rc.count() );

    minmaxfld_->setValue( rc.min(), 0 );
    minmaxfld_->setValue( rc.max(), 1 );
    avgstdfld_->setValue( rc.average(), 0 );
    avgstdfld_->setValue( rc.stdDev(), 1 );
    medrmsfld_->setValue( rc.median(), 0 );
    medrmsfld_->setValue( rc.rms(), 1 );
}


void uiStatsDisplay::setMarkValue( float val, bool forx )
{
    if ( histgramdisp_ )
	histgramdisp_->setMarkValue( val, forx );
}


void uiStatsDisplay::putN()
{
    if ( !histgramdisp_ )
	return;

    histgramdisp_->putN();
}


uiStatsDisplayWin::uiStatsDisplayWin( uiParent* p,
					const uiStatsDisplay::Setup& su,
					int nr, bool ismodal )
    : uiMainWin(p,"Data statistics",1,false,ismodal)
    , statnmcb_(0)
    , currentdispidx_(-1)
{
    uiLabeledComboBox* lblcb=0;
    if ( nr > 1 )
    {
	lblcb = new uiLabeledComboBox( this, "Select Data" );
	statnmcb_ = lblcb->box();
    }

    for ( int idx=0; idx<nr; idx++ )
    {
	uiStatsDisplay* disp = new uiStatsDisplay( this, su );
	if ( statnmcb_ )
	    disp->attach( rightAlignedBelow, lblcb );

	if ( disp->funcDisp() )
	{
	    disp->funcDisp()->getMouseEventHandler().movement.notify(
		mCB(this,uiStatsDisplayWin,mouseMoveCB) );
	}

	disps_ += disp;
	disp->display( false );
    }

    showStat( 0 );
    if ( statnmcb_ )
	statnmcb_->selectionChanged.notify(
		mCB(this,uiStatsDisplayWin,dataChanged) );
}


void uiStatsDisplayWin::showStat( int idx )
{
    currentdispidx_ = idx;
    if ( disps_.validIdx(idx) )
	disps_[idx]->display( true );
}


void uiStatsDisplayWin::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb)
    if ( !meh ) return;

    uiHistogramDisplay* disp = disps_.validIdx( currentdispidx_ )
		? disps_[currentdispidx_]->funcDisp() : 0;
    if ( !disp ) return;

    const Geom::Point2D<int>& pos = disp->getMouseEventHandler().event().pos();
    Geom::Point2D<float> val = disp->getFuncXY( pos.x, false );
    BufferString str;
    str.add( "Value/Count: ").add( toString(val.x,4) );
    str.add( " / " ).add( toString(val.y,0) );
    toStatusBar( str );
}


void uiStatsDisplayWin::dataChanged( CallBacker* )
{
    if ( !statnmcb_ )
	return;

    for ( int idx=0; idx<disps_.size(); idx++ )
	disps_[idx]->display( false );

    showStat( statnmcb_->currentItem() );
}


void uiStatsDisplayWin::setData( const float* medvals, int medsz, int dispidx )
{
    if ( disps_.validIdx(dispidx) )
	disps_[dispidx]->setData( medvals, medsz );
}


void uiStatsDisplayWin::addDataNames( const BufferStringSet& nms )
{ if ( statnmcb_ ) statnmcb_->addItems( nms ); }


void uiStatsDisplayWin::setDataName( const char* nm, int idx )
{
    if ( statnmcb_ )
    {
	idx > statnmcb_->size()-1 ? statnmcb_->addItem(nm)
				  : statnmcb_->setItemText(idx,nm);
	return;
    }

    if ( disps_.validIdx(idx) )
	disps_[idx]->setDataName( nm );
}


void uiStatsDisplayWin::setMarkValue( float val, bool forx, int idx )
{
    if ( disps_.validIdx(idx) )
	disps_[idx]->setMarkValue( val, forx );
}
