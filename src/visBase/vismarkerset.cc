/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          July 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vismarkerset.h"

#include "iopar.h"
#include "survinfo.h"
#include "vistransform.h"
#include "visosg.h"
#include "vismaterial.h"

#include <osgGeo/MarkerSet>

#include <math.h>

mCreateFactoryEntry( visBase::MarkerSet );

using namespace visBase;

MarkerSet::MarkerSet()
    : VisualObjectImpl(true)
    , markerset_( new osgGeo::MarkerSet )
    , displaytrans_( 0 )
    , coords_( Coordinates::create() )
    , singlecolormaterial_( new visBase::Material )
{
    markerset_->ref();
    addChild( markerset_ );
    markerset_->setVertexArray( mGetOsgVec3Arr(coords_->osgArray()) );

    setType( MarkerStyle3D::Cube );
    setScreenSize( cDefaultScreenSize() );
    setMaterial( 0 ); //Triggers update of markerset's color array
}


MarkerSet::~MarkerSet()
{
    clearMarkers();
    markerset_->unref();
}


Normals* MarkerSet::getNormals()
{
    if ( !normals_ )
    {
	normals_ = Normals::create();
	markerset_->setNormalArray( mGetOsgVec3Arr( normals_->osgArray()) );
    }
    
    return normals_;
}


void MarkerSet::setMaterial( visBase::Material* mat )
{
   if ( material_==mat ) return;

   if ( material_ )
       markerset_->setColorArray( 0 );
   else
       removeNodeState( singlecolormaterial_ );
   
   visBase::VisualObjectImpl::setMaterial( mat );
    if ( material_ )
	markerset_->setColorArray(
	    mGetOsgVec4Arr( material_->getColorArray() ) );
    else 
    {
	markerset_->setColorArray( 0 ); 
	addNodeState( (visBase::Material*) singlecolormaterial_ );
    }

}


void MarkerSet::clearMarkers()
{
    if ( coords_ ) coords_->setEmpty();
    if ( normals_ ) normals_->clear();
    if ( material_ ) material_->clear();
}


void MarkerSet::removeMarker( int idx )
{
    if ( coords_ ) coords_->removePos( idx );
    if ( normals_ ) normals_->removeNormal( idx );
    if ( material_ ) material_->removeColor( idx );
}


void MarkerSet::setMarkerStyle( const MarkerStyle3D& ms )
{
    singlecolormaterial_->setColor( ms.color_ );
    setType( ms.type_ );
    setScreenSize( (float) ms.size_ );
}


void MarkerSet::setMarkersSingleColor( const Color& singlecolor )
{
     singlecolormaterial_->setColor( singlecolor );
     setMaterial( 0 );
}


MarkerStyle3D::Type MarkerSet::getType() const
{
    return markerstyle_.type_;
}


void MarkerSet::setType( MarkerStyle3D::Type type )
{
    switch ( type )
    {
	case MarkerStyle3D::Cube:
	    markerset_->setShape( osgGeo::MarkerSet::Box );
	    break;
	case MarkerStyle3D::Cone:
	    markerset_->setShape( osgGeo::MarkerSet::Cone );
	    break;
	case MarkerStyle3D::Cylinder:
	    markerset_->setShape( osgGeo::MarkerSet::Cylinder );
	    break;
	case MarkerStyle3D::Sphere:
	    markerset_->setShape( osgGeo::MarkerSet::Sphere );
	    break;
	default:
	    pErrMsg("Shape not implemented");
	    markerset_->setShape( osgGeo::MarkerSet::None );
    }

    markerstyle_.type_ = type;
}


void MarkerSet::setScreenSize( float sz )
{
    markerstyle_.size_ = (int)sz;
    markerset_->setMarkerSize( sz );
}


float MarkerSet::getScreenSize() const
{
    return markerset_->getMarkerSize();
}


void MarkerSet::doFaceCamera(bool yn)
{
    markerset_->setRotateMode( yn
	? osg::AutoTransform::ROTATE_TO_CAMERA
	: osg::AutoTransform::NO_ROTATION );
}


void MarkerSet::setMarkerHeightRatio( float heightratio )
{
    markerset_->setMarkerHeightRatio( heightratio );
}


float MarkerSet::getMarkerHeightRatio() const 
{
    return markerset_->getMarkerHeightRatio();
}


void MarkerSet::setMaximumScale( float maxscale )
{
    markerset_->setMaxScale( maxscale );
}


float MarkerSet::getMaximumScale() const
{
    return markerset_->getMaxScale();
}


void MarkerSet::setMinimumScale( float minscale )
{
    markerset_->setMinScale( minscale );
}


float MarkerSet::getMinimumScale() const
{
    return markerset_->getMinScale();
}


void MarkerSet::setAutoRotateMode( AutoRotateMode rotatemode )
{
    markerset_->setRotateMode( (osg::AutoTransform::AutoRotateMode)rotatemode );
}


bool MarkerSet::facesCamera() const
{
    return markerset_->getRotateMode() == osg::AutoTransform::ROTATE_TO_CAMERA;
}


void MarkerSet::setDisplayTransformation( const mVisTrans* nt )
{
    coords_->setDisplayTransformation( nt );
    if ( normals_ ) normals_->setDisplayTransformation( nt );
    
    displaytrans_ = nt;
}


const mVisTrans* MarkerSet::getDisplayTransformation() const
{ return displaytrans_; }
