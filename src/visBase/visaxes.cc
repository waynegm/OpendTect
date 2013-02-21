/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		14-01-2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "visaxes.h"

#include <osg/Node> 
#include <osg/Geode>
#include <osg/Drawable>
#include <osgGeo/AxesNode>

mCreateFactoryEntry( visBase::Axes );


namespace visBase
{

Axes::Axes()
    : axesnode_(new osgGeo::AxesNode)
{
    setOsgNode( axesnode_ );
}


Axes::~Axes()
{
}

float Axes::getRadius() const
{
    return axesnode_->getRadius();
}


void Axes::setRadius( float rad )
{
    axesnode_->setRadius( rad );
}


} //namespace visBase
