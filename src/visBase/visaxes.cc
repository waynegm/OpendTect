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
    , ison_(true)
{
    axesnode_->ref();
}


Axes::~Axes()
{
    axesnode_->unref();
}


osg::Node* Axes::gtOsgNode()
{
    return axesnode_;
}


float Axes::getRadius() const
{
    return axesnode_->getRadius();
}


void Axes::setRadius( float rad )
{
    axesnode_->setRadius( rad );
}


bool Axes::turnOn( bool yn )
{
    const bool res = isOn();
    ison_ = yn;
    axesnode_->setNodeMask( ison_ ? 0xffffffff : 0x0 );
    return res;
}

} //namespace visBase
