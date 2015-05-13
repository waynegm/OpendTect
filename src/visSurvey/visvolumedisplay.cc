/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "visvolumedisplay.h"

#include "visboxdragger.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismarchingcubessurface.h"
#include "vismaterial.h"
#include "visselman.h"
#include "visrgbatexturechannel2rgba.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "visvolorthoslice.h"
#include "visvolrenscalarfield.h"

#include "array3dfloodfill.h"
#include "arrayndimpl.h"
#include "attribsel.h"
#include "binidvalue.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "trckeyzsampling.h"
#include "ioman.h"
#include "iopar.h"
#include "marchingcubes.h"
#include "picksettr.h"
#include "pickset.h"
#include "od_ostream.h"
#include "seisdatapack.h"
#include "settings.h"
#include "sorting.h"
#include "survinfo.h"
#include "zaxistransform.h"
#include "zaxistransformer.h"

#include <fstream>

/* OSG-TODO: Port VolrenDisplay volren_ and set of OrthogonalSlice slices_
   to OSG in case of prolongation. */


#define mVisMCSurf visBase::MarchingCubesSurface
#define mDefaultBoxTransparency 0.75



namespace visSurvey {

const char* VolumeDisplay::sKeyVolumeID()	{ return "Cube ID"; }
const char* VolumeDisplay::sKeyVolRen()		{ return "Volren"; }
const char* VolumeDisplay::sKeyInline()		{ return "Inline"; }
const char* VolumeDisplay::sKeyCrossLine()	{ return "Crossline"; }
const char* VolumeDisplay::sKeyTime()		{ return "Z-slice"; }

const char* VolumeDisplay::sKeyNrSlices()	{ return "Nr of slices"; }
const char* VolumeDisplay::sKeySlice()		{ return "SliceID "; }
const char* VolumeDisplay::sKeyTexture()	{ return "TextureID"; }

const char* VolumeDisplay::sKeyNrIsoSurfaces()	{ return "Nr Isosurfaces"; }
const char* VolumeDisplay::sKeyIsoValueStart()	{ return "Iso Value"; }
const char* VolumeDisplay::sKeyIsoOnStart()	{ return "Iso Surf On "; }
const char* VolumeDisplay::sKeySurfMode()	{ return "Surf Mode"; }
const char* VolumeDisplay::sKeySeedsMid()	{ return "Surf Seeds Mid"; }
const char* VolumeDisplay::sKeySeedsAboveIsov()	{ return "Above IsoVal"; }


static TrcKeyZSampling getInitTrcKeyZSampling( const TrcKeyZSampling& csin )
{
    TrcKeyZSampling cs(false);
    cs.hsamp_.start_.inl() = (5*csin.hsamp_.start_.inl()+3*csin.hsamp_.stop_.inl())/8;
    cs.hsamp_.start_.crl() = (5*csin.hsamp_.start_.crl()+3*csin.hsamp_.stop_.crl())/8;
    cs.hsamp_.stop_.inl() = (3*csin.hsamp_.start_.inl()+5*csin.hsamp_.stop_.inl())/8;
    cs.hsamp_.stop_.crl() = (3*csin.hsamp_.start_.crl()+5*csin.hsamp_.stop_.crl())/8;
    cs.zsamp_.start = ( 5*csin.zsamp_.start + 3*csin.zsamp_.stop ) / 8.f;
    cs.zsamp_.stop = ( 3*csin.zsamp_.start + 5*csin.zsamp_.stop ) / 8.f;
    SI().snap( cs.hsamp_.start_, BinID(0,0) );
    SI().snap( cs.hsamp_.stop_, BinID(0,0) );
    float z0 = csin.zsamp_.snap( cs.zsamp_.start ); cs.zsamp_.start = z0;
    float z1 = csin.zsamp_.snap( cs.zsamp_.stop ); cs.zsamp_.stop = z1;
    return cs;
}


VolumeDisplay::AttribData::AttribData()
    : as_( *new Attrib::SelSpec )
    , cache_( 0 )
{}


VolumeDisplay::AttribData::~AttribData()
{
    delete &as_;
    DPM( DataPackMgr::SeisID() ).release( cache_ );
}


VolumeDisplay::VolumeDisplay()
    : VisualObjectImpl(true)
    , boxdragger_(visBase::BoxDragger::create())
    , isinited_(0)
    , scalarfield_(0)
//    , volren_(0)
    , boxMoving(this)
    , datatransform_(0)
    , datatransformer_(0)
    , csfromsession_(true)
    , eventcatcher_( 0 )
    , onoffstatus_( true )
{
    addChild( boxdragger_->osgNode() );

    boxdragger_->ref();
    boxdragger_->setBoxTransparency( mDefaultBoxTransparency );
    mAttachCB( boxdragger_->started, VolumeDisplay::draggerStartCB );
    mAttachCB( boxdragger_->motion, VolumeDisplay::draggerMoveCB );
    mAttachCB( boxdragger_->finished, VolumeDisplay::draggerFinishCB );

    updateRanges( true, true );

    scalarfield_ = visBase::VolumeRenderScalarField::create();
    scalarfield_->ref();
    setChannels2RGBA( visBase::ColTabTextureChannel2RGBA::create() );
    addAttrib();

    addChild( scalarfield_->osgNode() );

    getMaterial()->setColor( Color::White() );
    getMaterial()->setAmbience( 0.3 );
    getMaterial()->setDiffIntensity( 0.8 );
    mAttachCB( getMaterial()->change, VolumeDisplay::materialChange );
    scalarfield_->setMaterial( getMaterial() );

    TrcKeyZSampling sics = SI().sampling( true );
    TrcKeyZSampling cs = getInitTrcKeyZSampling( sics );
    setTrcKeyZSampling( cs );

    int buttonkey = OD::NoButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyVolDepthKey(), buttonkey );
    boxdragger_->setPlaneTransDragKeys( true, buttonkey );
    buttonkey = OD::ShiftButton;
    mSettUse( get, "dTect.MouseInteraction", sKeyVolPlaneKey(), buttonkey );
    boxdragger_->setPlaneTransDragKeys( false, buttonkey );

    bool useindepthtransforresize = true;
    mSettUse( getYN, "dTect.MouseInteraction", sKeyInDepthVolResize(),
	      useindepthtransforresize );
    boxdragger_->useInDepthTranslationForResize( useindepthtransforresize );

    showManipulator( boxdragger_->isOn() );
}


VolumeDisplay::~VolumeDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );

    deepErase( attribs_ );

    TypeSet<int> children;
    getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	removeChild( children[idx] );

    boxdragger_->unRef();
    scalarfield_->unRef();

    setZAxisTransform( 0,0 );
}


void VolumeDisplay::setMaterial( visBase::Material* nm )
{
    if ( material_ )
	mDetachCB( material_->change, VolumeDisplay::materialChange );
    visBase::VisualObjectImpl::setMaterial( nm );
    if ( nm )
	mAttachCB( getMaterial()->change, VolumeDisplay::materialChange );
    materialChange( 0 );
}

#define mSetMaterialProp( prop ) \
    isosurfaces_[idx]->getMaterial()->set##prop( \
	    getMaterial()->get##prop() )
void VolumeDisplay::materialChange( CallBacker* )
{
    if ( !getMaterial() )
	return;

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	mSetMaterialProp( Ambience );
	mSetMaterialProp( DiffIntensity );
	mSetMaterialProp( SpecIntensity );
	mSetMaterialProp( EmmIntensity );
	mSetMaterialProp( Shininess );
	mSetMaterialProp( Transparency );
    }
}


void VolumeDisplay::updateIsoSurfColor()
{
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( mIsUdf( isosurfsettings_[idx].isovalue_) )
	    continue;

	const float val = isosurfsettings_[idx].isovalue_;
	Color col;
	if ( mIsUdf(val) )
	    col = getColTabSequence( 0 )->undefColor();
	else
	{
	    // TODO: adapt to multi-attrib
	    const float mappedval =
		scalarfield_->getColTabMapper(0).position( val );
	    col = getColTabSequence( 0 )->color( mappedval );
	}

	isosurfaces_[idx]->getMaterial()->setColor( col );
    }
}


bool VolumeDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    if ( zat == datatransform_ )
	return true;

    const bool haddatatransform = datatransform_;
    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	{
	    mDetachCB( *datatransform_->changeNotifier(),
		       VolumeDisplay::dataTransformCB );
	}
	datatransform_->unRef();
	datatransform_ = 0;
    }

    datatransform_ = zat;
    delete datatransformer_;
    datatransformer_ = 0;

    if ( datatransform_ )
    {
	datatransform_->ref();
	updateRanges( false, !haddatatransform );
	if ( datatransform_->changeNotifier() )
	{
	    mAttachCB( *datatransform_->changeNotifier(),
		       VolumeDisplay::dataTransformCB );
	}
    }

    return true;
}


const ZAxisTransform* VolumeDisplay::getZAxisTransform() const
{ return datatransform_; }


void VolumeDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
    boxdragger_->setRightHandSystem( yn );
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	isosurfaces_[idx]->setRightHandSystem( yn );
}


void VolumeDisplay::dataTransformCB( CallBacker* )
{
    updateRanges( false, true );
    for ( int attrib=0; attrib<attribs_.size(); attrib++ )
    {
	if ( attribs_[attrib]->cache_ )
	    setDataVolume( attrib, attribs_[attrib]->cache_, 0 );
    }
}


void VolumeDisplay::setScene( Scene* sc )
{
    SurveyObject::setScene( sc );
    if ( sc ) updateRanges( false, false );
}


void VolumeDisplay::updateRanges( bool updateic, bool updatez )
{
    if ( !datatransform_ ) return;

    const TrcKeyZSampling defcs( true );
    if ( csfromsession_ != defcs )
	setTrcKeyZSampling( csfromsession_ );
    else
    {
	const TrcKeyZSampling& csin = scene_ ? scene_->getTrcKeyZSampling()
					  : getTrcKeyZSampling( 0 );
	TrcKeyZSampling cs = getInitTrcKeyZSampling( csin );
	setTrcKeyZSampling( cs );
    }
}


void VolumeDisplay::getChildren( TypeSet<int>&res ) const
{
    res.erase();
    for ( int idx=0; idx<slices_.size(); idx++ )
	res += slices_[idx]->id();
    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	res += isosurfaces_[idx]->id();
//    if ( volren_ ) res += volren_->id();
}


void VolumeDisplay::showManipulator( bool yn )
{
    boxdragger_->turnOn( yn );
    scalarfield_->enableTraversal(visBase::cDraggerIntersecTraversalMask(),!yn);
}


bool VolumeDisplay::isManipulatorShown() const
{ return boxdragger_->isOn(); }


bool VolumeDisplay::isManipulated() const
{
    return !texturecs_.includes( getTrcKeyZSampling(true,true,0) );
}


bool VolumeDisplay::canResetManipulation() const
{ return true; }


void VolumeDisplay::resetManipulation()
{
}


void VolumeDisplay::acceptManipulation()
{
    setTrcKeyZSampling( getTrcKeyZSampling(true,true,0) );
}


void VolumeDisplay::draggerStartCB( CallBacker* )
{
    updateDraggerLimits( false );
}


void VolumeDisplay::draggerMoveCB( CallBacker* )
{
    TrcKeyZSampling cs = getTrcKeyZSampling(true,true,0);
    if ( scene_ )
	cs.limitTo( scene_->getTrcKeyZSampling() );

    const Coord3 center( (cs.hsamp_.start_.inl() + cs.hsamp_.stop_.inl())/2.0,
			 (cs.hsamp_.start_.crl() + cs.hsamp_.stop_.crl())/2.0,
			 (cs.zsamp_.start + cs.zsamp_.stop)/2.0 );

    const Coord3 width( cs.hsamp_.stop_.inl() - cs.hsamp_.start_.inl(),
			cs.hsamp_.stop_.crl() - cs.hsamp_.start_.crl(),
			cs.zsamp_.stop - cs.zsamp_.start );

    boxdragger_->setCenter( center );
    boxdragger_->setWidth( width );

    setTrcKeyZSampling( cs, true );
    if ( keepdraggerinsidetexture_ )
    {
	boxdragger_->setBoxTransparency( 1.0 );
	boxdragger_->showScaleTabs( false );
    }
    boxMoving.trigger();
}


void VolumeDisplay::draggerFinishCB( CallBacker* )
{
    boxdragger_->setBoxTransparency( mDefaultBoxTransparency );
    boxdragger_->showScaleTabs( true );
}


int VolumeDisplay::addSlice( int dim )
{
    visBase::OrthogonalSlice* slice = visBase::OrthogonalSlice::create();
    slice->ref();
    slice->setMaterial(0);
    slice->setDim(dim);
    mAttachCB( slice->motion, VolumeDisplay::sliceMoving );
    slices_ += slice;

    slice->setName( dim==cTimeSlice() ? sKeyTime() :
		   (dim==cCrossLine() ? sKeyCrossLine() : sKeyInline()) );

    addChild( slice->osgNode() );
    const TrcKeyZSampling cs = getTrcKeyZSampling( 0 );
    const Interval<float> defintv(-0.5,0.5);
    slice->setSpaceLimits( defintv, defintv, defintv );
    // TODO: adapt to multi-attrib
    if ( attribs_[0]->cache_ )
    {
	const Array3D<float>& arr = attribs_[0]->cache_->data();
	slice->setVolumeDataSize( arr.info().getSize(2),
				  arr.info().getSize(1),
				  arr.info().getSize(0) );
    }

    return slice->id();
}


void VolumeDisplay::removeChild( int displayid )
{
/*
    if ( volren_ && displayid==volren_->id() )
    {
	VisualObjectImpl::removeChild( volren_->osgNode() );
	volren_->unRef();
	volren_ = 0;
	return;
    }
*/

    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	if ( slices_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild( slices_[idx]->osgNode() );
	    mDetachCB( slices_[idx]->motion, VolumeDisplay::sliceMoving );
	    slices_.removeSingle(idx,false)->unRef();
	    return;
	}
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	if ( isosurfaces_[idx]->id()==displayid )
	{
	    VisualObjectImpl::removeChild(
		    isosurfaces_[idx]->osgNode() );

	    isosurfaces_.removeSingle(idx,false)->unRef();
	    isosurfsettings_.removeSingle(idx,false);
	    return;
	}
    }
}


void VolumeDisplay::showVolRen( bool yn )
{
/*
    if ( yn && !volren_ )
    {
	volren_ = visBase::VolrenDisplay::create();
	volren_->ref();
	volren_->setMaterial(0);
	addChild( volren_->osgNode() );
	volren_->setName( sKeyVolRen() );
    }

    if ( volren_ ) volren_->turnOn( yn );
*/
}


bool VolumeDisplay::isVolRenShown() const
{
    return false;
//    return volren_ && volren_->isOn();
}


float VolumeDisplay::defaultIsoValue() const
{
    // TODO: adapt to multi-attrib
    return attribs_[0]->cache_ ? getColTabMapperSetup(0)->range_.center()
			       : mUdf(float);
}


int VolumeDisplay::addIsoSurface( TaskRunner* tr, bool updateisosurface )
{
    mVisMCSurf* isosurface = mVisMCSurf::create();
    isosurface->ref();
    isosurface->setRightHandSystem( righthandsystem_ );
    mDeclareAndTryAlloc( RefMan<MarchingCubesSurface>, surface,
			 MarchingCubesSurface() );
    isosurface->setSurface( *surface, tr );
    isosurface->setName( "Iso surface" );

    isosurfaces_ += isosurface;
    IsosurfaceSetting setting;
    setting.isovalue_ = defaultIsoValue();
    isosurfsettings_ += setting;

    if ( updateisosurface )
	updateIsoSurface( isosurfaces_.size()-1, tr );

    //add before the volume transform
    addChild( isosurface->osgNode() );
    materialChange( 0 ); //updates new surface's material
    return isosurface->id();
}


int VolumeDisplay::volRenID() const
{
    return -1;
//    return volren_ ? volren_->id() : -1;
}


#define mSetVolumeTransform( name, center, width, trans, scale ) \
\
    Coord3 trans( center ); \
    mVisTrans::transform( displaytrans_, trans ); \
    Coord3 scale( width ); \
    mVisTrans::transformSize( displaytrans_, scale ); \
    trans += 0.5 * scale; \
    scale = Coord3( scale.z, -scale.y, -scale.x ); \
    scalarfield_->set##name##Transform( trans, Coord3(0,1,0), M_PI_2, scale );

void VolumeDisplay::setTrcKeyZSampling( const TrcKeyZSampling& desiredcs,
				     bool dragmode )
{
    TrcKeyZSampling cs( desiredcs );

    if ( dragmode )
	cs.limitTo( texturecs_ );
    else if ( scene_ )
	cs.limitTo( scene_->getTrcKeyZSampling() );

    const Coord3 center( (cs.hsamp_.start_.inl() + cs.hsamp_.stop_.inl())/2.0,
			 (cs.hsamp_.start_.crl() + cs.hsamp_.stop_.crl())/2.0,
			 (cs.zsamp_.start + cs.zsamp_.stop)/2.0 );

    const Coord3 width( cs.hsamp_.stop_.inl() - cs.hsamp_.start_.inl(),
			cs.hsamp_.stop_.crl() - cs.hsamp_.start_.crl(),
			cs.zsamp_.stop - cs.zsamp_.start );

    const Coord3 step( cs.hsamp_.step_.inl(), cs.hsamp_.step_.crl(), cs.zsamp_.step );

    updateDraggerLimits( dragmode );
    mSetVolumeTransform( ROIVolume, center, width, trans, scale );

    if ( dragmode )
	return;

    mSetVolumeTransform( TexVolume, center, width+step, textrans, texscale );
    texturecs_ = cs;
    scalarfield_->turnOn( false );

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	isosurfaces_[idx]->getSurface()->removeAll();
	isosurfaces_[idx]->touch( false );
    }

    boxdragger_->setCenter( center );
    boxdragger_->setWidth( width );
}


void VolumeDisplay::updateDraggerLimits( bool dragmode )
{
    const TrcKeyZSampling curcs = getTrcKeyZSampling( true, true, 0 );

    if ( !dragmode )
    {
	keepdraggerinsidetexture_ = false;
	draggerstartcs_ = curcs;
    }

    if ( curcs!=draggerstartcs_ && texturecs_.includes(curcs) &&
	 scalarfield_->isOn() )
    {
	keepdraggerinsidetexture_ = true;
    }

    TrcKeyZSampling limcs( texturecs_ );
    if ( !keepdraggerinsidetexture_ && scene_ )
	limcs = scene_->getTrcKeyZSampling();

    const Interval<float> inlrg( mCast(float,limcs.hsamp_.start_.inl()),
				 mCast(float,limcs.hsamp_.stop_.inl()) );
    const Interval<float> crlrg( mCast(float,limcs.hsamp_.start_.crl()),
				 mCast(float,limcs.hsamp_.stop_.crl()) );

    boxdragger_->setSpaceLimits( inlrg, crlrg, limcs.zsamp_ );

    const int minvoxwidth = 1;
    boxdragger_->setWidthLimits(
	Interval<float>( mCast(float,minvoxwidth*limcs.hsamp_.step_.inl()),
			 mUdf(float) ),
	Interval<float>( mCast(float,minvoxwidth*limcs.hsamp_.step_.crl()),
			 mUdf(float) ),
	Interval<float>( minvoxwidth*limcs.zsamp_.step, mUdf(float) ) );
}


float VolumeDisplay::getValue( int attrib, const Coord3& pos ) const
{
    if ( !attribs_.validIdx(attrib) || !attribs_[attrib]->cache_ )
	return mUdf(float);

    const BinID bid( SI().transform(pos) );
    const TrcKeyZSampling& samp = attribs_[attrib]->cache_->sampling();
    const int inlidx = samp.inlIdx( bid.inl() );
    const int crlidx = samp.crlIdx( bid.crl() );
    const int zidx = samp.zsamp_.getIndex( pos.z );

    const float val =
	attribs_[attrib]->cache_->data().get( inlidx, crlidx, zidx );
    return val;
}


float VolumeDisplay::isoValue( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    return idx<0 ? mUdf(float) : isosurfsettings_[idx].isovalue_;
}


void VolumeDisplay::setIsoValue( const mVisMCSurf* mcd, float nv,
				 TaskRunner* tr )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 )
	return;

    isosurfsettings_[idx].isovalue_ = nv;
    updateIsoSurface( idx, tr );
}


mVisMCSurf* VolumeDisplay::getIsoSurface( int idx )
{ return isosurfaces_.validIdx(idx) ? isosurfaces_[idx] : 0; }


int VolumeDisplay::getNrIsoSurfaces()
{ return isosurfaces_.size(); }


char VolumeDisplay::isFullMode( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
	return -1;

    return isosurfsettings_[idx].mode_;
}


void VolumeDisplay::setFullMode( const mVisMCSurf* mcd, bool full )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 )
	return;

    isosurfsettings_[idx].mode_ = full ? 1 : 0;
}


char VolumeDisplay::seedAboveIsovalue( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
            return -1;

    return isosurfsettings_[idx].seedsaboveisoval_;
}


void VolumeDisplay::setSeedAboveIsovalue( const mVisMCSurf* mcd, bool above )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
            return;

    isosurfsettings_[idx].seedsaboveisoval_ = above;
}


MultiID  VolumeDisplay::getSeedsID( const mVisMCSurf* mcd ) const
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
	return MultiID();

    return isosurfsettings_[idx].seedsid_;
}


void VolumeDisplay::setSeedsID( const mVisMCSurf* mcd, MultiID mid )
{
    const int idx = isosurfaces_.indexOf( mcd );
    if ( idx<0 || idx>=isosurfaces_.size() )
	return;

    isosurfsettings_[idx].seedsid_ = mid;
}


bool VolumeDisplay::updateSeedBasedSurface( int idx, TaskRunner* tr )
{
    // TODO: adapt to multi-attrib
    if ( idx<0 || idx>=isosurfaces_.size() || !attribs_[0]->cache_ ||
	 mIsUdf(isosurfsettings_[idx].isovalue_) ||
	 isosurfsettings_[idx].seedsid_.isEmpty() )
	return false;

    Pick::Set seeds;
    if ( Pick::Mgr().indexOf(isosurfsettings_[idx].seedsid_)!=-1 )
	seeds = Pick::Mgr().get( isosurfsettings_[idx].seedsid_ );
    else
    {
	BufferString ermsg;
	if ( !PickSetTranslator::retrieve( seeds,
		    IOM().get(isosurfsettings_[idx].seedsid_), true, ermsg ) )
	    return false;
    }

    // TODO: adapt to multi-attrib
    const Array3D<float>& data = attribs_[0]->cache_->data();
    if ( !data.isOK() )
	return false;

    Array3DImpl<float> newarr( data.info() );
    Array3DFloodfill<float> ff( data, isosurfsettings_[idx].isovalue_,
	    isosurfsettings_[idx].seedsaboveisoval_, newarr );
    ff.useInputValue( true );

    TrcKeyZSampling cs = getTrcKeyZSampling(true,true,0);
    cs.normalise();
    for ( int seedidx=0; seedidx<seeds.size(); seedidx++ )
    {
	const Coord3 pos =  seeds[seedidx].pos_;
	const BinID bid = SI().transform( pos );
	const int i = cs.inlIdx( bid.inl() );
	const int j = cs.crlIdx( bid.crl() );
	const int k = cs.zIdx( (float) pos.z );
	ff.addSeed( i, j, k );
    }

    if ( !ff.execute() )
	return false;

    if ( isosurfsettings_[idx].seedsaboveisoval_ )
    {
	const float outsideval = 1;
	const float threshold = isosurfsettings_[idx].isovalue_;
	float* newdata = newarr.getData();
	if ( newdata )
	{
            for ( od_int64 idy=newarr.info().getTotalSz()-1; idy>=0; idy-- )
		newdata[idy] = mIsUdf(newdata[idy]) ? outsideval
						    : threshold - newdata[idy];
	}
        else if ( newarr.getStorage() )
        {
            ValueSeries<float>* newstor = newarr.getStorage();
            for ( od_int64 idy=newarr.info().getTotalSz()-1; idy>=0; idy-- )
            {
		newstor->setValue(idy,
                    mIsUdf(newstor->value(idy))
                        ? outsideval
                        : threshold - newstor->value(idy) );
            }

        }
	else
	{
	    for ( int id0=0; id0<newarr.info().getSize(0); id0++ )
            {
		for ( int idy=0; idy<newarr.info().getSize(1); idy++ )
                {
		    for ( int idz=0; idz<newarr.info().getSize(2); idz++ )
		    {
			float val = newarr.get(id0,idy,idz);
			val = mIsUdf(val) ? outsideval : threshold-val;
			newarr.set( id0, idy, idz, val );
		    }
                }
            }
	}
    }

    const float threshold = isosurfsettings_[idx].seedsaboveisoval_
	? 0
	: isosurfsettings_[idx].isovalue_;

    isosurfaces_[idx]->getSurface()->setVolumeData( 0, 0, 0, newarr,
						    threshold, tr );
    return true;
}


int VolumeDisplay::getIsoSurfaceIdx( const mVisMCSurf* mcd ) const
{
    return isosurfaces_.indexOf( mcd );
}


void VolumeDisplay::updateIsoSurface( int idx, TaskRunner* tr )
{
    // TODO:: adapt to multi-attrib
    const RegularSeisDataPack* cache = attribs_[0]->cache_;
    if ( !cache || cache->isEmpty() ||
	 mIsUdf(isosurfsettings_[idx].isovalue_) )
	isosurfaces_[idx]->getSurface()->removeAll();
    else
    {
	const TrcKeyZSampling& samp = cache->sampling();
	isosurfaces_[idx]->getSurface()->removeAll();
	isosurfaces_[idx]->setBoxBoundary(
		mCast(float,samp.hsamp_.inlRange().stop),
		mCast(float,samp.hsamp_.crlRange().stop),
		samp.zsamp_.stop );

	const SamplingData<float> inlsampling(
		mCast(float,samp.hsamp_.inlRange().start),
		mCast(float,samp.hsamp_.inlRange().step) );

	const SamplingData<float> crlsampling(
		mCast(float,samp.hsamp_.crlRange().start),
		mCast(float,samp.hsamp_.crlRange().step) );

	SamplingData<float> zsampling ( samp.zsamp_.start, samp.zsamp_.step );
	isosurfaces_[idx]->setScales( inlsampling, crlsampling, zsampling );

	if ( isosurfsettings_[idx].mode_ )
	    isosurfaces_[idx]->getSurface()->setVolumeData( 0, 0, 0,
		    cache->data(), isosurfsettings_[idx].isovalue_, tr );
	else
	{
	    if ( !updateSeedBasedSurface( idx, tr ) )
		return;
	}

    }

    updateIsoSurfColor();
    isosurfaces_[idx]->touch( false, tr );
}


BufferString VolumeDisplay::getManipulationString() const
{
    BufferString str;
    getObjectInfo( str );
    return str;
}


void VolumeDisplay::getObjectInfo( BufferString& info ) const
{
    TrcKeyZSampling cs = getTrcKeyZSampling( true, true, 0 );
    info = "Inl: ";
    info += cs.hsamp_.start_.inl(); info += "-"; info += cs.hsamp_.stop_.inl();
    info += ", Crl: ";
    info += cs.hsamp_.start_.crl(); info += "-"; info += cs.hsamp_.stop_.crl();
    info += ", ";

    float zstart = cs.zsamp_.start;
    float zstop = cs.zsamp_.stop;

    if ( scene_ )
    {
	info += scene_->zDomainInfo().userName();
	info += ": ";
	zstart *= scene_->zDomainInfo().userFactor();
	zstop *= scene_->zDomainInfo().userFactor();
    }


    info += mNINT32(zstart); info += "-"; info += mNINT32(zstop);
}


void VolumeDisplay::getTreeObjectInfo( BufferString& info ) const
{
    TrcKeyZSampling cs = getTrcKeyZSampling( true, true, 0 );
    cs.limitTo( texturecs_ );

    if ( !cs.isEmpty() && scalarfield_->isOn() )
    {
	info += cs.hsamp_.start_.inl(); info += "-"; info += cs.hsamp_.stop_.inl();
	info += ", ";
	info += cs.hsamp_.start_.crl(); info += "-"; info += cs.hsamp_.stop_.crl();
	info += ", ";

	float zstart = cs.zsamp_.start;
	float zstop = cs.zsamp_.stop;

	if ( scene_ )
	{
	    zstart *= scene_->zDomainInfo().userFactor();
	    zstop *= scene_->zDomainInfo().userFactor();
	}

	info += mNINT32(zstart); info += "-"; info += mNINT32(zstop);
    }
    else
	info = "<empty>";
}


void VolumeDisplay::sliceMoving( CallBacker* cb )
{
    mDynamicCastGet( visBase::OrthogonalSlice*, slice, cb );
    if ( !slice ) return;

    slicename_ = slice->name();
    sliceposition_ = slicePosition( slice );
}


float VolumeDisplay::slicePosition( visBase::OrthogonalSlice* slice ) const
{
    if ( !slice ) return 0;
    const int dim = slice->getDim();
    float slicepos = slice->getPosition();
//    slicepos *= (float) -voltrans_->getScale()[dim];

    float pos;
    if ( dim == 2 )
    {
//	slicepos += (float) voltrans_->getTranslation()[0];
	pos = mCast( float, SI().inlRange(true).snap(slicepos) );
    }
    else if ( dim == 1 )
    {
//	slicepos += (float) voltrans_->getTranslation()[1];
	pos = mCast( float, SI().crlRange(true).snap(slicepos) );
    }
    else
    {
//	slicepos += (float) voltrans_->getTranslation()[2];
	pos = slicepos;
    }

    return pos;
}


void VolumeDisplay::setSlicePosition( visBase::OrthogonalSlice* slice,
					const TrcKeyZSampling& cs )
{
    if ( !slice ) return;

    const int dim = slice->getDim();
    float pos = 0;
    Interval<float> rg;
    int nrslices = 0;
    slice->getSliceInfo( nrslices, rg );
    if ( dim == 2 )
	pos = (float)cs.hsamp_.inlRange().start;
    else if ( dim == 1 )
	pos = (float)cs.hsamp_.crlRange().start;
    else
	pos = (float)cs.zsamp_.start;

//    pos -= (float) voltrans_->getTranslation()[2-dim];
//    pos /= (float) -voltrans_->getScale()[dim];

    float slicenr =  nrslices ? (pos-rg.start)*nrslices/rg.width() : 0;
    float draggerpos = slicenr /(nrslices-1) *rg.width() + rg.start;
    Coord3 center(0,0,0);
    center[dim] = draggerpos;
    slice->setCenter( center, false );
    slice->motion.trigger();
}


const TypeSet<float>* VolumeDisplay::getHistogram( int attrib ) const
{ return &scalarfield_->getHistogram( attrib ); }


SurveyObject::AttribFormat VolumeDisplay::getAttributeFormat( int ) const
{ return visSurvey::SurveyObject::Cube; }


const Attrib::SelSpec* VolumeDisplay::getSelSpec( int attrib ) const
{
    return attribs_.validIdx(attrib) ? &attribs_[attrib]->as_ : 0;
}


void VolumeDisplay::setSelSpec( int attrib, const Attrib::SelSpec& as )
{
    if ( !attribs_.validIdx(attrib) || attribs_[attrib]->as_==as )
	return;

    attribs_[attrib]->as_ = as;
    DPM( DataPackMgr::SeisID() ).release( attribs_[attrib]->cache_ );
    attribs_[attrib]->cache_ = 0;

    scalarfield_->setScalarField( attrib, 0, true, 0 );
    updateAttribEnabling();

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	updateIsoSurface( idx );
}


TrcKeyZSampling VolumeDisplay::getTrcKeyZSampling( int attrib ) const
{ return getTrcKeyZSampling(true,false,attrib); }


bool VolumeDisplay::setDataPackID( int attrib, DataPack::ID dpid,
				   TaskRunner* tr )
{
    if ( !attribs_.validIdx(attrib) )
	return false;

    DataPackMgr& dpman = DPM( DataPackMgr::SeisID() );
    const DataPack* datapack = dpman.obtain( dpid );
    mDynamicCastGet(const RegularSeisDataPack*,cdp,datapack);
    const bool res = setDataVolume( attrib, cdp ? cdp : 0, tr );
    return res;
}


bool VolumeDisplay::setDataVolume( int attrib,
				   const RegularSeisDataPack* attribdata,
				   TaskRunner* tr )
{
    if ( !attribs_.validIdx(attrib) || !attribdata )
	return false;

    const Array3D<float>* usedarray = 0;
    bool arrayismine = true;
    if ( alreadyTransformed(attrib) || !datatransform_ )
	usedarray = &attribdata->data();
    else
    {
	if ( !datatransformer_ )
	    mTryAlloc( datatransformer_,ZAxisTransformer(*datatransform_,true));

//	datatransformer_->setInterpolate( !isClassification(attrib) );
	datatransformer_->setInterpolate( true );
	datatransformer_->setInput( attribdata->data(),
				    attribdata->sampling() );
	datatransformer_->setOutputRange( getTrcKeyZSampling(true,true,0) );

	if ( !TaskRunner::execute( tr, *datatransformer_ ) )
	{
	    pErrMsg( "Transform failed" );
	    return false;
	}

	usedarray = datatransformer_->getOutput( true );
	if ( !usedarray )
	{
	    pErrMsg( "No output from transform" );
	    return false;
	}

	arrayismine = false;
    }

    scalarfield_->setScalarField( attrib, usedarray, !arrayismine, tr );

    setTrcKeyZSampling( getTrcKeyZSampling(true,true,0) );

    for ( int idx=0; idx<slices_.size(); idx++ )
	slices_[idx]->setVolumeDataSize( usedarray->info().getSize(2),
					 usedarray->info().getSize(1),
					 usedarray->info().getSize(0) );

    if ( attribs_[attrib]->cache_ != attribdata )
    {
	DPM( DataPackMgr::SeisID() ).release( attribs_[attrib]->cache_ );
	attribs_[attrib]->cache_ = attribdata;
	DPM( DataPackMgr::SeisID() ).obtain( attribs_[attrib]->cache_->id() );
    }

    updateAttribEnabling();

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
	updateIsoSurface( idx );

    return true;
}


const RegularSeisDataPack* VolumeDisplay::getCacheVolume( int attrib ) const
{
    return attribs_.validIdx(attrib) ? attribs_[attrib]->cache_ : 0;
}


DataPack::ID VolumeDisplay::getDataPackID( int attrib ) const
{
    DataPack::ID dpid = DataPack::cNoID();
    if ( attribs_.validIdx(attrib) && attribs_[attrib]->cache_ )
	dpid = attribs_[attrib]->cache_->id();

    return dpid;
}


void VolumeDisplay::getMousePosInfo( const visBase::EventInfo&,
				     Coord3& pos, BufferString& val,
				     BufferString& info ) const
{
    info = "";
    val = "undef";
    Coord3 attribpos = pos;
    ConstRefMan<ZAxisTransform> datatrans = getZAxisTransform();
    if ( datatrans ) //TODO check for allready transformed data.
    {
	attribpos.z = datatrans->transformBack( pos );
	if ( !attribpos.isDefined() )
	    return;
    }

    if ( !isManipulatorShown() )
	val = getValue( 0, attribpos ); // TODO: adapt to multi-attrib
}


TrcKeyZSampling VolumeDisplay::getTrcKeyZSampling( bool manippos,
						   bool displayspace,
						   int attrib ) const
{
    TrcKeyZSampling res = texturecs_;
    if ( manippos )
    {
	Coord3 center = boxdragger_->center();
	Coord3 width = boxdragger_->width();

	res.hsamp_.start_ = BinID( mNINT32( center.x - width.x/2 ),
			      mNINT32( center.y - width.y/2 ) );

	res.hsamp_.stop_ = BinID( mNINT32( center.x + width.x/2 ),
			     mNINT32( center.y + width.y/2 ) );

	res.hsamp_.step_ = BinID( SI().inlStep(), SI().crlStep() );

	res.zsamp_.start = (float) ( center.z - width.z/2 );
	res.zsamp_.stop = (float) ( center.z + width.z/2 );
	res.zsamp_.step = SI().zStep();

	SI().snap( res.hsamp_.start_ );
	SI().snap( res.hsamp_.stop_ );

	if ( !datatransform_ )
	{
	    SI().snapZ( res.zsamp_.start );
	    SI().snapZ( res.zsamp_.stop );
	}
	else
	{
	    StepInterval<float> zrg = datatransform_->getZInterval(false);
	    if ( scene_ )
		zrg.step = scene_->getTrcKeyZSampling().zsamp_.step;
	    else
		zrg.step = datatransform_->getGoodZStep();

	    zrg.snap( res.zsamp_.start );
	    zrg.snap( res.zsamp_.stop );
	}
    }

    const bool alreadytf = alreadyTransformed( attrib );
    if ( alreadytf )
    {
	if ( scene_ )
	    res.zsamp_.step = scene_->getTrcKeyZSampling().zsamp_.step;
	else if ( datatransform_ )
	    res.zsamp_.step = datatransform_->getGoodZStep();
	return res;
    }

    if ( datatransform_ )
    {
	if ( !displayspace )
	{
	    res.zsamp_.setFrom( datatransform_->getZInterval(true) );
	    res.zsamp_.step = SI().zRange(true).step;
	}
	else
	{
	    if ( scene_ )
		res.zsamp_.step = scene_->getTrcKeyZSampling().zsamp_.step;
	    else
		res.zsamp_.step = datatransform_->getGoodZStep();
	}
    }
    else
	res.zsamp_.step = SI().zRange(true).step;

    return res;
}


bool VolumeDisplay::allowsPicks() const
{
    return !isVolRenShown();
}


visSurvey::SurveyObject* VolumeDisplay::duplicate( TaskRunner* tr ) const
{
    VolumeDisplay* vd = new VolumeDisplay;

    TypeSet<int> children;
    vd->getChildren( children );
    for ( int idx=0; idx<children.size(); idx++ )
	vd->removeChild( children[idx] );

    vd->setZAxisTransform( const_cast<ZAxisTransform*>(datatransform_), tr );
    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	const int sliceid = vd->addSlice( slices_[idx]->getDim() );
	mDynamicCastGet(visBase::OrthogonalSlice*,slice,
			visBase::DM().getObject(sliceid));
	slice->setSliceNr( slices_[idx]->getSliceNr() );
    }

    for ( int idx=0; idx<isosurfaces_.size(); idx++ )
    {
	vd->addIsoSurface();
	vd->isosurfsettings_[idx] = isosurfsettings_[idx];
    }

    vd->showVolRen( isVolRenShown() );

    vd->setTrcKeyZSampling( getTrcKeyZSampling(false,true,0) );

    for ( int attrib=0; attrib<attribs_.size(); attrib++ )
    {
	while ( attrib >= vd->nrAttribs() )
	    vd->addAttrib();

	vd->setSelSpec( attrib, attribs_[attrib]->as_ );
	vd->setDataVolume( attrib, attribs_[attrib]->cache_, tr );
	vd->setColTabMapperSetup( attrib,
			    scalarfield_->getColTabMapper(attrib).setup_, tr );
	vd->setColTabSequence( attrib,
			       *getChannels2RGBA()->getSequence(attrib), tr );
    }

    return vd;
}


void VolumeDisplay::setSceneEventCatcher( visBase::EventCatcher* ec )
{
    if ( eventcatcher_ )
    {
	mDetachCB( eventcatcher_->eventhappened,
		   VolumeDisplay::updateMouseCursorCB );
	eventcatcher_->unRef();
    }

    eventcatcher_ = ec;

    if ( eventcatcher_ )
    {
	eventcatcher_->ref();
	mAttachCB( eventcatcher_->eventhappened,
		   VolumeDisplay::updateMouseCursorCB );
    }
}


bool VolumeDisplay::isSelected() const
{
    return visBase::DM().selMan().selected().indexOf( id()) != -1;
}


void VolumeDisplay::updateMouseCursorCB( CallBacker* cb )
{
    if ( !isManipulatorShown() || !isOn() || isLocked() )
	mousecursor_.shape_ = MouseCursor::NotSet;
    else
	initAdaptiveMouseCursor( cb, id(),
		    boxdragger_->getPlaneTransDragKeys(false), mousecursor_ );
}


VolumeDisplay::IsosurfaceSetting::IsosurfaceSetting()
{
    mode_ = 1;
    seedsaboveisoval_ = -1;
    seedsid_ = MultiID();
}


bool VolumeDisplay::IsosurfaceSetting::operator==(
	const IsosurfaceSetting& ns ) const
{
    return mode_==ns.mode_ && seedsaboveisoval_==ns.seedsaboveisoval_ &&
	   seedsid_==ns.seedsid_ &&
	   mIsEqual(isovalue_, ns.isovalue_, (isovalue_+ns.isovalue_)/2000 );
}


VolumeDisplay::IsosurfaceSetting& VolumeDisplay::IsosurfaceSetting::operator=(
       const IsosurfaceSetting& ns )
{
    mode_ = ns.mode_;
    seedsaboveisoval_ = ns.seedsaboveisoval_;
    seedsid_ = ns.seedsid_;
    isovalue_ = ns.isovalue_;

    return *this;
}


bool VolumeDisplay::canSetColTabSequence() const
{
    mDynamicCastGet( const visBase::RGBATextureChannel2RGBA*,
		     rgba2rgba, getChannels2RGBA() );
    return !rgba2rgba;
}


void VolumeDisplay::setColTabSequence( int attrib, const ColTab::Sequence& seq,
					TaskRunner* tr )
{
    if ( getChannels2RGBA() )
    {
	getChannels2RGBA()->setSequence( attrib, seq );
	scalarfield_->makeColorTables( attrib );
	updateIsoSurfColor();
    }
}


const ColTab::Sequence* VolumeDisplay::getColTabSequence( int attrib ) const
{
    return getChannels2RGBA() ? getChannels2RGBA()->getSequence(attrib) : 0;
}


void VolumeDisplay::setColTabMapperSetup( int attrib,
					  const ColTab::MapperSetup& ms,
					  TaskRunner* tr )
{
    scalarfield_->setColTabMapperSetup( attrib, ms, tr );
    updateIsoSurfColor();
}


const ColTab::MapperSetup* VolumeDisplay::getColTabMapperSetup( int attrib,
							    int version ) const
{
    return &scalarfield_->getColTabMapper(attrib).setup_;
}


bool VolumeDisplay::turnOn( bool yn )
{
    onoffstatus_ = yn;

    return VisualObjectImpl::turnOn( isAnyAttribEnabled() && yn );
}


bool VolumeDisplay::isOn() const
{ return onoffstatus_; }


void VolumeDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );
    const TrcKeyZSampling cs = getTrcKeyZSampling(false,true,0);
    cs.fillPar( par );

    pErrMsg( "Not implemented" );
}


bool VolumeDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	return false;

    PtrMan<IOPar> texturepar = par.subselect( sKeyTexture() );
    if ( texturepar ) //old format (up to 4.0)
    {
	ColTab::MapperSetup mappersetup;
	ColTab::Sequence sequence;

	mappersetup.usePar(*texturepar);
	sequence.usePar(*texturepar );
	setColTabMapperSetup( 0, mappersetup, 0 );
	setColTabSequence( 0, sequence, 0 );
	if ( !attribs_[0]->as_.usePar(par) )
	    return false;
    }

    int volid;
    if ( par.get(sKeyVolumeID(),volid) )
    {
	RefMan<visBase::DataObject> dataobj = visBase::DM().getObject( volid );
	if ( !dataobj ) return false;
/*
	mDynamicCastGet(visBase::VolrenDisplay*,vr,dataobj.ptr());
	if ( !vr ) return -1;

	{
	    if ( childIndex(volren_->osgNode())!=-1 )
		VisualObjectImpl::removeChild(volren_->osgNode());
	    volren_->unRef();
	}
	volren_ = vr;
	volren_->ref();
	addChild( volren_->osgNode() );
*/
    }

    while ( slices_.size() )
	removeChild( slices_[0]->id() );

    while ( isosurfaces_.size() )
	removeChild( isosurfaces_[0]->id() );

    int nrslices = 0;
    par.get( sKeyNrSlices(), nrslices );
    for ( int idx=0; idx<nrslices; idx++ )
    {
	BufferString str( sKeySlice(), idx );
	int sliceid;
	par.get( str, sliceid );
	RefMan<visBase::DataObject> dataobj = visBase::DM().getObject(sliceid);
	if ( !dataobj ) return 0;
	mDynamicCastGet(visBase::OrthogonalSlice*,os,dataobj.ptr())
	if ( !os ) return -1;
	os->ref();
	mAttachCB( os->motion, VolumeDisplay::sliceMoving );
	slices_ += os;
	addChild( os->osgNode() );
	// set correct dimensions ...
	if ( os->name()==sKeyInline() )
	    os->setDim( cInLine() );
	else if ( os->name()==sKeyCrossLine() )
	    os->setDim( cCrossLine() );
	else if ( os->name()==sKeyTime() )
	    os->setDim( cTimeSlice() );
    }

    TrcKeyZSampling cs;
    if ( cs.usePar(par) )
    {
	csfromsession_ = cs;
	setTrcKeyZSampling( cs );
    }

    int nrisosurfaces;
    if ( par.get( sKeyNrIsoSurfaces(), nrisosurfaces ) )
    {
	for ( int idx=0; idx<nrisosurfaces; idx++ )
	{
	    BufferString str( sKeyIsoValueStart() ); str += idx;
	    float isovalue;
	    if ( par.get( str, isovalue ) )
	    {
		addIsoSurface( 0, false );
		isosurfsettings_[idx].isovalue_ = isovalue;
	    }

	    str = sKeyIsoOnStart(); str += idx;
	    bool status = true;
	    par.getYN( str, status );
	    isosurfaces_[idx]->turnOn( status );

	    str = sKeySurfMode(); str += idx;
	    int smode;
	    par.get( str, smode );
	    isosurfsettings_[idx].mode_ = mCast( char, smode );

	    str = sKeySeedsAboveIsov(); str += idx;
	    int aboveisov;
	    par.get( str, aboveisov );
	    isosurfsettings_[idx].seedsaboveisoval_ = mCast( char, aboveisov );

	    str = sKeySeedsMid(); str += idx;
	    MultiID mid;
	    par.get( str, mid );
	    isosurfsettings_[idx].seedsid_ = mid;
	}
    }

    return true;
}


bool VolumeDisplay::writeVolume( int attrib, const char* filename ) const
{
    if ( !attribs_.validIdx(attrib) || !scalarfield_ )
	return false;

    od_ostream strm( filename );
    if ( !strm.isOK() )
    {
	errmsg_ = "Cannot open file";
	return false;
    }

    errmsg_ = scalarfield_->writeVolumeFile( attrib, strm );
    return errmsg_.str();
}


visBase::OrthogonalSlice* VolumeDisplay::getSelectedSlice() const
{
    for ( int idx=0; idx<slices_.size(); idx++ )
    {
	if ( slices_[idx]->isSelected() )
	    return const_cast<visBase::OrthogonalSlice*>( slices_[idx] );
    }

    return 0;
}


TrcKeyZSampling
	VolumeDisplay::sliceSampling(visBase::OrthogonalSlice* slice) const
{
    TrcKeyZSampling cs(false);
    if ( !slice ) return cs;
    cs = getTrcKeyZSampling(false,true,0);
    float pos = slicePosition( slice );
    if ( slice->getDim() == cTimeSlice() )
	cs.zsamp_.limitTo( Interval<float>( pos, pos ) );
    else if ( slice->getDim() == cCrossLine() )
	cs.hsamp_.setCrlRange( Interval<int>( mNINT32(pos), mNINT32(pos) ) );
    else if ( slice->getDim() == cInLine() )
	cs.hsamp_.setInlRange( Interval<int>( mNINT32(pos), mNINT32(pos) ) );
    return cs;
}


void VolumeDisplay::setDisplayTransformation( const mVisTrans* t )
{
    const bool voldisplayed = scalarfield_->isOn();
    TrcKeyZSampling cs = getTrcKeyZSampling( false, true, 0 );

    displaytrans_ = t;
    boxdragger_->setDisplayTransformation( t );

    setTrcKeyZSampling( cs );
    scalarfield_->turnOn( voldisplayed );
}


bool VolumeDisplay::canEnableTextureInterpolation() const
{ return true; }


bool VolumeDisplay::textureInterpolationEnabled() const
{ return scalarfield_->textureInterpolationEnabled(); }


void VolumeDisplay::enableTextureInterpolation( bool yn )
{ scalarfield_->enableTextureInterpolation( yn ); }


bool VolumeDisplay::canUseVolRenShading()
{
    return visBase::VolumeRenderScalarField::isShadingSupported();
}


void VolumeDisplay::allowShading( bool yn )
{
    scalarfield_->allowShading( yn );
}


bool VolumeDisplay::usesShading() const
{ return scalarfield_->usesShading(); }


bool VolumeDisplay::canAddAttrib( int nr ) const
{
    if ( !getChannels2RGBA() )
	return false;

    mDynamicCastGet( const visBase::ColTabTextureChannel2RGBA*, coltabtc2rgba,
		     getChannels2RGBA() );
    if ( coltabtc2rgba )	// Multiple coltab textures not yet supported
	return nrAttribs()+nr <= 1;

    return nrAttribs()+nr <= getChannels2RGBA()->maxNrChannels();
}


bool VolumeDisplay::canRemoveAttrib() const
{
    if ( !getChannels2RGBA() )
	return false;

    return nrAttribs() > getChannels2RGBA()->minNrChannels();
}


int VolumeDisplay::nrAttribs() const
{ return attribs_.size(); }


bool VolumeDisplay::addAttrib()
{
    if ( !canAddAttrib() )
	return false;

    const int attrib = attribs_.size();
    attribs_ += new AttribData();
    getChannels2RGBA()->notifyChannelInsert( attrib );
    enableAttrib( attrib, isAttribEnabled(attrib) );
    return true;
}


bool VolumeDisplay::removeAttrib( int attrib )
{
    if ( !canRemoveAttrib() || !attribs_.validIdx(attrib) )
	return false;

    getChannels2RGBA()->notifyChannelRemove( attrib );
    delete attribs_.removeSingle( attrib );
    updateAttribEnabling();
    turnOn( onoffstatus_ );
    return true;
}


void VolumeDisplay::updateAttribEnabling()
{
    bool showvolren = false;

    if ( getChannels2RGBA() )
    {
	for ( int idx=0; idx<attribs_.size(); idx++ )
	{
	    bool yn = getChannels2RGBA()->isEnabled( idx );

	    if ( !attribs_[idx]->cache_ )
		yn = false;

	    scalarfield_->enableAttrib( idx, yn );
	    showvolren = showvolren || yn;
	}
    }

    scalarfield_->turnOn( onoffstatus_ && showvolren );
}


void VolumeDisplay::enableAttrib( int attrib, bool yn )
{
    if ( getChannels2RGBA() )
	getChannels2RGBA()->setEnabled( attrib, yn );

    updateAttribEnabling();
    turnOn( onoffstatus_ );
}


bool VolumeDisplay::isAttribEnabled( int attrib ) const
{
    return getChannels2RGBA() ? getChannels2RGBA()->isEnabled(attrib) : false;
}


bool VolumeDisplay::swapAttribs( int attrib0, int attrib1 )
{
    if ( !attribs_.validIdx(attrib0) || !attribs_.validIdx(attrib1) ||
	 attrib0==attrib1 || !getChannels2RGBA() )
	return false;

    getChannels2RGBA()->swapChannels( attrib0, attrib1 );
    attribs_.swap( attrib0, attrib1 );

    scalarfield_->swapAttribs( attrib0, attrib1 );

    updateAttribEnabling();
    return true;
}


void VolumeDisplay::setAttribTransparency( int attrib, unsigned char trans )
{
    mDynamicCastGet( visBase::ColTabTextureChannel2RGBA*,
		     coltab2rgba, getChannels2RGBA() );
    if ( coltab2rgba )
	coltab2rgba->setTransparency( attrib, trans );

    mDynamicCastGet( visBase::RGBATextureChannel2RGBA*,
		     rgba2rgba, getChannels2RGBA() );
    if ( rgba2rgba )
	rgba2rgba->setTransparency( trans );

    scalarfield_->setAttribTransparency( attrib, trans );
}


unsigned char VolumeDisplay::getAttribTransparency( int attrib ) const
{
    mDynamicCastGet( const visBase::ColTabTextureChannel2RGBA*,
		     coltab2rgba, getChannels2RGBA() );
    if ( coltab2rgba )
	return coltab2rgba->getTransparency( attrib );

    mDynamicCastGet( const visBase::RGBATextureChannel2RGBA*,
		     rgba2rgba, getChannels2RGBA() );
    if ( rgba2rgba )
	return rgba2rgba->getTransparency();

    return 0;
}


bool VolumeDisplay::setChannels2RGBA( visBase::TextureChannel2RGBA* tc2rgba )
{
    if ( scalarfield_ )
	scalarfield_->setChannels2RGBA( tc2rgba );

    return scalarfield_;
}


visBase::TextureChannel2RGBA* VolumeDisplay::getChannels2RGBA()
{
    return scalarfield_ ? scalarfield_->getChannels2RGBA() : 0;
}


const visBase::TextureChannel2RGBA* VolumeDisplay::getChannels2RGBA() const
{
    return scalarfield_ ? scalarfield_->getChannels2RGBA() : 0;
}


} // namespace visSurvey
