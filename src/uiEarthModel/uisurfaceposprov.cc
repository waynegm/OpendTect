/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/


#include "uisurfaceposprov.h"
#include "emsurfaceposprov.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uiselsurvranges.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "emsurfacetr.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "keystrs.h"
#include "ioobj.h"
#include "iopar.h"


uiSurfacePosProvGroup::uiSurfacePosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , ctio1_(*mMkCtxtIOObj(EMHorizon3D))
    , ctio2_(*mMkCtxtIOObj(EMHorizon3D))
    , zfac_(mCast(float,SI().zDomain().userFactor()))
    , zstepfld_(0)
    , extrazfld_(0)
    , surf1fld_(0)
    , surf2fld_(0)
{
    if ( su.is2d_ )
    {
	new uiLabel( this, tr("Not implemented for 2D") );
	return;
    }
    surf1fld_ = new uiIOObjSel( this, ctio1_, uiStrings::sHorizon() );

    const CallBack selcb( mCB(this,uiSurfacePosProvGroup,selChg) );
    issingfld_ = new uiGenInput( this, uiStrings::sSelect(),
			BoolInpSpec(true,tr("On Horizon"),
                                    tr("To a 2nd Horizon")) );
    issingfld_->attach( alignedBelow, surf1fld_ );
    issingfld_->valuechanged.notify( selcb );

    surf2fld_ = new uiIOObjSel( this, ctio2_, uiStrings::sBottomHor() );
    surf2fld_->attach( alignedBelow, issingfld_ );

    uiString txt;
    if ( su.withstep_ )
    {
	zstepfld_ = new uiSpinBox( this, 0, "Z step" );
	zstepfld_->attach( alignedBelow, surf2fld_ );
	float zstep = SI().zRange(OD::UsrWork).step * 10;
	int v = (int)((zstep * zfac_) + .5);
	zstepfld_->setValue( v );
	zstepfld_->setInterval( StepInterval<int>(1,999999,1) );
	txt = tr("Z step").withSurvZUnit();
	zsteplbl_ = new uiLabel( this, txt, zstepfld_ );
    }

    if ( su.withz_ )
    {
	extrazfld_ = new uiSelZRange( this, false, true, tr("Extra Z") );
	if ( zstepfld_ )
	    extrazfld_->attach( alignedBelow, zstepfld_ );
	else
	    extrazfld_->attach( alignedBelow, surf2fld_ );
    }

    setHAlignObj( surf1fld_ );
    postFinalise().notify( selcb );
}


uiSurfacePosProvGroup::~uiSurfacePosProvGroup()
{
    delete ctio1_.ioobj_; delete &ctio1_;
    delete ctio2_.ioobj_; delete &ctio2_;
}


void uiSurfacePosProvGroup::selChg( CallBacker* )
{
    const bool isbtwn = !issingfld_->getBoolValue();
    surf2fld_->display( isbtwn );
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mGetSurfKey(k) \
    IOPar::compKey(sKey::Surface(),Pos::EMSurfaceProvider3D::k##Key())


void uiSurfacePosProvGroup::usePar( const IOPar& iop )
{
    if ( !surf1fld_ ) return;

    surf1fld_->setInput( DBKey(iop.find(mGetSurfKey(id1))) );
    const char* res = iop.find( mGetSurfKey(id2) );
    const bool issing = !res || !*res;
    if ( !issing )
	surf2fld_->setInput( DBKey(res) );
    issingfld_->setValue( issing );

    if ( zstepfld_ )
    {
	float zstep = zstepfld_->getFValue() / zfac_;
	iop.get( mGetSurfKey(zstep), zstep );
	int v = (int)((zstep * zfac_) + .5);
	zstepfld_->setValue( v );
    }

    if ( extrazfld_ )
    {
	StepInterval<float> ez = extrazfld_->getRange();
	iop.get( mGetSurfKey(extraZ), ez );
	extrazfld_->setRange( ez );
    }

    selChg( 0 );
}


bool uiSurfacePosProvGroup::fillPar( IOPar& iop ) const
{
    if ( !surf1fld_ ) return false;

    if ( !surf1fld_->commitInput() )
	mErrRet(uiStrings::phrSelect(uiStrings::sSurface().toLower()))
    iop.set( mGetSurfKey(id1), ctio1_.ioobj_->key() );

    Interval<float> ez( 0, 0 );
    if ( issingfld_->getBoolValue() )
	iop.removeWithKey( mGetSurfKey(id2) );
    else
    {
	if ( !surf2fld_->commitInput() )
	    mErrRet(uiStrings::phrSelect(tr("the bottom horizon")))
	 if (  ctio2_.ioobj_->key() ==	ctio1_.ioobj_->key() )
	     mErrRet(uiStrings::phrSelect(tr("two different horizons")))
	iop.set( mGetSurfKey(id2), ctio2_.ioobj_->key() );
    }

    const float zstep = zstepfld_ ? zstepfld_->getFValue() / zfac_
				  : SI().zStep();
    iop.set( mGetSurfKey(zstep), zstep );

    if ( mIsUdf(ez.start) ) ez.start = 0;
    if ( mIsUdf(ez.stop) ) ez.stop = 0;

    if ( extrazfld_ ) assign( ez, extrazfld_->getRange() );
    iop.set( mGetSurfKey(extraZ), ez );
    iop.set( sKey::Type(), sKey::Surface() );
    return true;
}


void uiSurfacePosProvGroup::getSummary( uiString& txt ) const
{
    if ( !surf1fld_ ) return;
    txt.appendPhrase(issingfld_->getBoolValue() ? tr("On Horizon") :
		tr("Between Horizons"), uiString::Space, uiString::OnSameLine);
}


void uiSurfacePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Surface(),
					  uiStrings::sHorizon() );
}
