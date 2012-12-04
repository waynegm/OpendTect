/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vispolygonoffset.h"

#include <osg/PolygonOffset>


namespace visBase
{


PolygonOffset::PolygonOffset()
    : offset_( addAttribute( new osg::PolygonOffset ) )
{
    offset_->ref();
}


PolygonOffset::~PolygonOffset()
{
    offset_->unref();
}


void PolygonOffset::setFactor( float f )
{
    offset_->setFactor( f );
}


float PolygonOffset::getFactor() const
{
    return offset_->getFactor();
}


void PolygonOffset::setUnits( float f )
{
    offset_->setUnits( f );
}


float PolygonOffset::getUnits() const
{
    return offset_->getUnits();
}


}; //namespace
