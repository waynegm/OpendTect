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

#include <osgGeo/MarkerSet>

#include <math.h>

mCreateFactoryEntry( visBase::MarkerSet );

using namespace visBase;

MarkerSet::MarkerSet()
    : VisualObjectImpl(true)
    , markerset_( new osgGeo::MarkerSet )
    , displaytrans_( 0 )
    , coords_( Coordinates::create() )
{
    setOsgNode( markerset_ );
    
    markerset_->setVertexArray( mGetOsgVec3Arr(coords_->osgArray()) );
    setType( MarkerStyle3D::Cube );
    setScreenSize( cDefaultScreenSize() );
}


MarkerSet::~MarkerSet()
{}


Normals* MarkerSet::getNormals()
{
    if ( !normals_ )
    {
	normals_ = Normals::create();
	markerset_->setNormalArray( mGetOsgVec3Arr( normals_->osgArray()) );
    }
    
    return normals_;
}


void MarkerSet::setMarkerStyle( const MarkerStyle3D& ms )
{
    setType( ms.type_ );
    setScreenSize( (float)ms.size_ );
}


MarkerStyle3D::Type MarkerSet::getType() const
{
    return markerstyle_.type_;
}

/*
static float getSurveyRotation()
{
    const RCol2Coord& b2c = SI().binID2Coord();
    const float xcrd = (float) b2c.getTransform(true).c;
    const float ycrd = (float) b2c.getTransform(false).c;
    const float angle = atan2( ycrd, xcrd );
    return angle;
}
 */


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
	    /*
	case MarkerStyle3D::Arrow:
	    markerset_->setShape( osgGeo::MarkerSet::Arrow );
	    break;
	case MarkerStyle3D::Cross:
	    markerset_->setShape( osgGeo::MarkerSet::Cross );
	    break;
	case MarkerStyle3D::Plane:
	    markerset_->setShape( osgGeo::MarkerSet::Cross );
	    break;
	     */
	default:
	    pErrMsg("Shape not implemented");
	    markerset_->setShape( osgGeo::MarkerSet::None );
    }

    markerstyle_.type_ = type;
}


void MarkerSet::setScreenSize( float sz )
{
    markerset_->setScreenSize( sz );
    markerstyle_.size_ = (int)sz;
}


float MarkerSet::getScreenSize() const
{
    return markerset_->getScreenSize();
}


void MarkerSet::doFaceCamera(bool yn)
{
    markerset_->setRotateMode( yn
	? osg::AutoTransform::ROTATE_TO_CAMERA
	: osg::AutoTransform::NO_ROTATION );
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
