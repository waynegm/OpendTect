/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: visdragger.cc 26837 2012-10-17 09:36:11Z kristofer.tingdahl@dgbes.com $";


#include "visthumbwheel.h"

#include "visevent.h"
#include "vistransform.h"

#include <osg/Switch>
#include <osgGeo/ThumbWheel>
#include <osg/MatrixTransform>
#include <osgGA/GUIEventAdapter>

mCreateFactoryEntry( visBase::ThumbWheel );

namespace visBase
{

ThumbWheel::ThumbWheel()
    : positiontransform_( new osg::MatrixTransform )
    , onoff_( new osg::Group )
{
    onoff_->addChild( positiontransform_ );
    positiontransform_->setName( "Wheel PosTransform");
    
    thumbwheel_ =  new osgGeo::ThumbWheel;
    thumbwheel_->setName( "Thumbwheel");
    thumbwheel_->setIntersectionMask( IntersectionTraversal );
    thumbwheel_->setActivationMouseButtonMask(
			      osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON );

    initDragger( thumbwheel_ );
    positiontransform_->addChild( thumbwheel_ );
}


osg::Node* ThumbWheel::gtOsgNode()
{
    return onoff_;
}
    

ThumbWheel::~ThumbWheel()
{
    if ( displaytrans_ ) displaytrans_->unRef();
}


}; // namespace visBase
