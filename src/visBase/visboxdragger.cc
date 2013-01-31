/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visboxdragger.h"

#include "vistransform.h"
#include "ranges.h"
#include "iopar.h"
#include "survinfo.h"

#include <osgManipulator/TabBoxDragger>
#include <osg/Switch>

mCreateFactoryEntry( visBase::BoxDragger );


namespace visBase
{


class BoxDraggerCallbackHandler: public osgManipulator::DraggerCallback
{

public: 
				BoxDraggerCallbackHandler( BoxDragger& dragger )
				    : dragger_( dragger )
				{}

    using			osgManipulator::DraggerCallback::receive;
    virtual bool		receive(const osgManipulator::MotionCommand&);

protected:

    void			constrain();

    BoxDragger&			dragger_;
    osg::Matrix			initialosgmatrix_;
    Coord3			initialcenter_;
};


bool BoxDraggerCallbackHandler::receive(
				    const osgManipulator::MotionCommand& cmd )
{
    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	initialosgmatrix_ = dragger_.osgboxdragger_->getMatrix();
	initialcenter_ = dragger_.center();
    }

    mDynamicCastGet( const osgManipulator::Scale1DCommand*, s1d, &cmd );
    mDynamicCastGet( const osgManipulator::Scale2DCommand*, s2d, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInPlaneCommand*,
		     translatedinplane, &cmd );

    if ( !s1d && !s2d && !translatedinplane )
    {
	dragger_.osgboxdragger_->setMatrix( initialosgmatrix_ );
	return true;
    }

    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
    {
	dragger_.started.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::MOVE )
    {
	constrain();
	dragger_.motion.trigger();
    }
    else if ( cmd.getStage()==osgManipulator::MotionCommand::FINISH )
    {
	dragger_.finished.trigger();
	if ( initialosgmatrix_ != dragger_.osgboxdragger_->getMatrix() )
	    dragger_.changed.trigger();
    }

    return true;
}


void BoxDraggerCallbackHandler::constrain()
{
    Coord3 scale = dragger_.width();
    Coord3 center = dragger_.center();

    for ( int dim=0; dim<3; dim++ )
    {
	if ( dragger_.spaceranges_[dim].width(false) > 0.0 )
	{
	    double diff = center[dim] - 0.5*scale[dim] -
			  dragger_.spaceranges_[dim].start;
	    if ( diff < 0.0 )
	    {
		center[dim] -= 0.5*diff;
		scale[dim] += diff;
	    }

	    diff = center[dim] + 0.5*scale[dim] -
		   dragger_.spaceranges_[dim].stop;
	    if ( diff > 0.0 )
	    {
		center[dim] -= 0.5*diff;
		scale[dim] -= diff;
	    }
	}

	if ( dragger_.widthranges_[dim].width(false) > 0.0 )
	{
	    double diff = scale[dim] - dragger_.widthranges_[dim].start;
	    if ( diff < 0 )
	    {
		if ( center[dim] < initialcenter_[dim] )
		    center[dim] -= 0.5*diff;
		else
		    center[dim] += 0.5*diff;

		scale[dim] -= diff;
	    }

	    diff = scale[dim] - dragger_.widthranges_[dim].stop;
	    if ( diff > 0 )
	    {
		if ( center[dim] > initialcenter_[dim] )
		    center[dim] -= 0.5*diff;
		else
		    center[dim] += 0.5*diff;

		scale[dim] -= diff;
	    }
	}
    }

    dragger_.setOsgMatrix( scale, center );
}


//=============================================================================


BoxDragger::BoxDragger()
    : started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , osgboxdragger_( 0 )
    , osgdraggerroot_( 0 )
    , osgcallbackhandler_( 0 )
{
    osgdraggerroot_ = new osg::Switch();
    osgdraggerroot_->ref();

    osgboxdragger_ = new osgManipulator::TabBoxDragger();
    osgdraggerroot_->addChild( osgboxdragger_ );

    osgboxdragger_->setupDefaultGeometry();
    osgboxdragger_->setHandleEvents( true );

    osgcallbackhandler_ = new BoxDraggerCallbackHandler( *this );
    osgboxdragger_->addDraggerCallback( osgcallbackhandler_ );

    setBoxTransparency( 0.0 );

    for ( int idx=osgboxdragger_->getNumDraggers()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( osgManipulator::TabPlaneDragger*, tpd,
			 osgboxdragger_->getDragger(idx) );

	for ( int idy=tpd->getNumDraggers()-1; idy>=0; idy-- )
	{
	    osgManipulator::Dragger* dragger = tpd->getDragger( idy );
	    mDynamicCastGet( osgManipulator::Scale1DDragger*, s1dd, dragger );
	    if ( s1dd )
	    {
		s1dd->setColor( osg::Vec4(0.0,0.7,0.0,1.0) );
		s1dd->setPickColor( osg::Vec4(0.0,1.0,0.0,1.0) );
	    }
	    mDynamicCastGet( osgManipulator::Scale2DDragger*, s2dd, dragger );
	    if ( s2dd )
	    {
		s2dd->setColor( osg::Vec4(0.0,0.7,0.0,1.0) );
		s2dd->setPickColor( osg::Vec4(0.0,1.0,0.0,1.0) );
	    }
	}
    }

    osgboxdragger_->getOrCreateStateSet()->setAttributeAndModes(
	    new osg::PolygonOffset(-1.0,1.0), osg::StateAttribute::ON );
}


BoxDragger::~BoxDragger()
{
    osgboxdragger_->removeDraggerCallback( osgcallbackhandler_ );
    osgdraggerroot_->unref();
}


void BoxDragger::setOsgMatrix( const Coord3& worldscale,
			       const Coord3& worldtrans )
{
    osg::Vec3d scale, trans;
    mVisTrans::transformDir( transform_, worldscale, scale );
    mVisTrans::transform( transform_, worldtrans, trans ); 

    osg::Matrix mat;
    mat *= osg::Matrix::scale( scale );
    mat *= osg::Matrix::translate( trans );
    osgboxdragger_->setMatrix( mat );
}


void BoxDragger::setBoxTransparency( float transparency )
{
    osgboxdragger_->setPlaneColor( osg::Vec4(0.7,0.7,0.7,1.0-transparency) );
}


void BoxDragger::setCenter( const Coord3& pos )
{
    setOsgMatrix( width(), pos );
}


Coord3 BoxDragger::center() const
{
    Coord3 trans;
    mVisTrans::transformBack( transform_,
			      osgboxdragger_->getMatrix().getTrans(),
			      trans );
    return trans;
}


void BoxDragger::setWidth( const Coord3& scale )
{
    setOsgMatrix( scale, center() );
}


Coord3 BoxDragger::width() const
{
    Coord3 scale;
    mVisTrans::transformBackDir( transform_,
				 osgboxdragger_->getMatrix().getScale(),
				 scale );

    scale.x = fabs(scale.x); scale.y = fabs(scale.y); scale.z = fabs(scale.z);
    return scale;
}


void BoxDragger::setSpaceLimits( const Interval<float>& x,
				 const Interval<float>& y,
				 const Interval<float>& z )
{
    spaceranges_[0] = x; spaceranges_[1] = y; spaceranges_[2] = z;

}


void BoxDragger::setWidthLimits( const Interval<float>& x,
				 const Interval<float>& y,
				 const Interval<float>& z )
{
    widthranges_[0] = x; widthranges_[1] = y; widthranges_[2] = z;
}


bool BoxDragger::turnOn( bool yn )
{
    const bool res = isOn();
    osgdraggerroot_->setValue( 0, yn );
    return res;
}


bool BoxDragger::isOn() const
{
    return osgdraggerroot_->getValue(0);
}


osg::Node* BoxDragger::gtOsgNode()
{ return osgdraggerroot_; }


void BoxDragger::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transform_ == nt )
	return;

    const Coord3 oldcenter = center();
    const Coord3 oldwidth = width();

    transform_ = nt;

    setWidth( oldwidth );
    setCenter( oldcenter );
}


const mVisTrans* BoxDragger::getDisplayTransformation() const
{
    return transform_;
}


}; // namespace visBase
