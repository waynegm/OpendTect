/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visboxdragger.h"
#include "ranges.h"
#include "iopar.h"
#include "survinfo.h"

#include <osgManipulator/TabBoxDragger>
#include <osg/Switch>

mCreateFactoryEntry( visBase::BoxDragger );


namespace visBase
{


static void setOsgMatrix( osgManipulator::TabBoxDragger& osgdragger,
			  const osg::Vec3& scale, const osg::Vec3& trans )
{
    osg::Matrix mat;
    mat *= osg::Matrix::scale( scale );
    mat *= osg::Matrix::translate( trans );
    osgdragger.setMatrix( mat );
}


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
    osg::Matrix			startmatrix_;
};


bool BoxDraggerCallbackHandler::receive(
				    const osgManipulator::MotionCommand& cmd )
{
    if ( cmd.getStage()==osgManipulator::MotionCommand::START )
	startmatrix_ = dragger_.osgboxdragger_->getMatrix();

    mDynamicCastGet( const osgManipulator::Scale1DCommand*, s1d, &cmd );
    mDynamicCastGet( const osgManipulator::Scale2DCommand*, s2d, &cmd );
    mDynamicCastGet( const osgManipulator::TranslateInPlaneCommand*,
		     translatedinplane, &cmd );

    if ( !s1d && !s2d && !translatedinplane )
    {
	dragger_.osgboxdragger_->setMatrix( startmatrix_ );
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
	if ( startmatrix_ != dragger_.osgboxdragger_->getMatrix() )
	    dragger_.changed.trigger();
    }

    return true;
}


void BoxDraggerCallbackHandler::constrain()
{
    osg::Vec3 scale = dragger_.osgboxdragger_->getMatrix().getScale();
    osg::Vec3 center = dragger_.osgboxdragger_->getMatrix().getTrans();

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
		if ( center[dim] < startmatrix_.getTrans()[dim] )
		    center[dim] -= 0.5*diff;
		else
		    center[dim] += 0.5*diff;

		scale[dim] -= diff;
	    }

	    diff = scale[dim] - dragger_.widthranges_[dim].stop;
	    if ( diff > 0 )
	    {
		if ( center[dim] > startmatrix_.getTrans()[dim] )
		    center[dim] -= 0.5*diff;
		else
		    center[dim] += 0.5*diff;

		scale[dim] -= diff;
	    }
	}
    }

    setOsgMatrix( *dragger_.osgboxdragger_, scale, center );
}


//=============================================================================


BoxDragger::BoxDragger()
    : started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , xinterval_( 0 )
    , yinterval_( 0 )
    , zinterval_( 0 )
    , selectable_( false )
    , osgboxdragger_( 0 )
    , osgdraggerroot_( 0 )
    , osgcallbackhandler_( 0 )
{
    initOsgDragger();
}


BoxDragger::~BoxDragger()
{
    if ( osgdraggerroot_ && osgboxdragger_ )
    {
	osgboxdragger_->removeDraggerCallback( osgcallbackhandler_ );
	osgdraggerroot_->unref();
    }

    delete xinterval_;
    delete yinterval_;
    delete zinterval_;
}


void BoxDragger::initOsgDragger()
{
    if ( osgboxdragger_ )
	return;

    osgdraggerroot_ = new osg::Switch();
    osgdraggerroot_->ref();

    osgboxdragger_ = new osgManipulator::TabBoxDragger();
    osgdraggerroot_->addChild( osgboxdragger_ );

    osgboxdragger_->setupDefaultGeometry();
    osgboxdragger_->setHandleEvents( true );
    setBoxTransparency( 0.0 );

    osgcallbackhandler_ = new BoxDraggerCallbackHandler( *this );
}


void BoxDragger::setBoxTransparency( float f )
{
    if ( osgboxdragger_ )
	osgboxdragger_->setPlaneColor( osg::Vec4(0.7,0.7,0.7,1.0-f) );
}


void BoxDragger::setCenter( const Coord3& pos )
{
    prevcenter_ = pos;

    if ( osgboxdragger_ )
    {
	setOsgMatrix( *osgboxdragger_, osgboxdragger_->getMatrix().getScale(),
		      osg::Vec3(pos.x,pos.y,pos.z) );
    }
}


Coord3 BoxDragger::center() const
{
    
    osg::Vec3 dragcenter = osgboxdragger_->getMatrix().getTrans();
    return Coord3( dragcenter[0], dragcenter[1], dragcenter[2] );
}


void BoxDragger::setWidth( const Coord3& pos )
{
    prevwidth_ = pos;


    setOsgMatrix( *osgboxdragger_, osg::Vec3f(pos.x,pos.y,pos.z),	
		      osgboxdragger_->getMatrix().getTrans() );
}


Coord3 BoxDragger::width() const
{
    
    osg::Vec3 boxwidth = osgboxdragger_->getMatrix().getScale();
    return Coord3( boxwidth[0], boxwidth[1], boxwidth[2] );
}


void BoxDragger::setSpaceLimits( const Interval<float>& x,
				 const Interval<float>& y,
				 const Interval<float>& z )
{
    if ( !xinterval_ )
    {
	xinterval_ = new Interval<float>(x);
	yinterval_ = new Interval<float>(y);
	zinterval_ = new Interval<float>(z);
	return;
    }

    *xinterval_ = x;
    *yinterval_ = y;
    *zinterval_ = z;

    spaceranges_[0] = x; spaceranges_[1] = y; spaceranges_[2] = z;

}


void BoxDragger::setWidthLimits( const Interval<float>& x,
				 const Interval<float>& y,
				 const Interval<float>& z )
{
    widthranges_[0] = x; widthranges_[1] = y; widthranges_[2] = z;
}


void BoxDragger::turnOn( bool yn )
{
    osgdraggerroot_->setValue( 0, yn );
}


bool BoxDragger::isOn() const
{
    return osgdraggerroot_->getValue(0);
}


osg::Node* BoxDragger::gtOsgNode()
{ return osgdraggerroot_; }


}; // namespace visBase
