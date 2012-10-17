/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "visdragger.h"

#include "visevent.h"
#include "vistransform.h"

#include <osg/Switch>
#include <osgManipulator/Translate1DDragger>
#include <osgManipulator/Translate2DDragger>
#include <osg/MatrixTransform>

mCreateFactoryEntry( visBase::Dragger );

namespace visBase
{
    
    
class DraggerCallbackHandler : public osgManipulator::DraggerCallback
{
public:
    DraggerCallbackHandler( DraggerBase& dragger )
	: dragger_( dragger )
    {}
    
    using		osgManipulator::DraggerCallback::receive;
    bool		receive(const osgManipulator::MotionCommand&);
protected:
    
    DraggerBase&	dragger_;
};

    

    
DraggerBase::DraggerBase()
    : started( this )
    , motion( this )
    , finished( this )
    , displaytrans_( 0 )
{}
    
    
DraggerBase::~DraggerBase()
{ if ( displaytrans_ ) displaytrans_->unRef(); }

    
void DraggerBase::setDisplayTransformation( const mVisTrans* nt )
{
    if ( displaytrans_ )
    {
	displaytrans_->unRef();
	displaytrans_ = 0;
    }
    
    displaytrans_ = nt;
    if ( displaytrans_ )
    {
	displaytrans_->ref();
    }
}


const mVisTrans* DraggerBase::getDisplayTransformation() const
{
    return displaytrans_;
}



Dragger::Dragger()
    : positiontransform_( new osg::MatrixTransform )
    , rightclicknotifier_(this)
    , rightclickeventinfo_( 0 )
    , onoff_( new osg::Switch )
{
    onoff_->addChild( positiontransform_ );
    setDefaultRotation();
}


void Dragger::setDefaultRotation()
{
    setRotation( Coord3(0,1,0), -M_PI_2 );
}
    
    
osg::Node* Dragger::osgNode()
{
    return onoff_;
}
    

Dragger::~Dragger()
{
}


bool Dragger::selectable() const { return true; }


void Dragger::setDraggerType( Type tp )
{
    positiontransform_->removeChild( dragger_ );

    switch ( tp )
    {
	case Translate1D:
	    dragger_ = new osgManipulator::Translate1DDragger;
	    break;
	case Translate2D:
	    dragger_ = new osgManipulator::Translate2DDragger;
	    break;
	case Translate3D:
	case Scale3D:
	    pErrMsg("Not impl");
    };

    positiontransform_->addChild( dragger_ );
    pErrMsg("Setup callbacks");
}


void Dragger::setOwnShape( DataObject* newshape, const char* partname )
{
    pErrMsg("Not impl");
}


void Dragger::triggerRightClick( const EventInfo* eventinfo )
{
    rightclickeventinfo_ = eventinfo;
    rightclicknotifier_.trigger();
}


const TypeSet<int>* Dragger::rightClickedPath() const
{ return rightclickeventinfo_ ? &rightclickeventinfo_->pickedobjids : 0; }


const EventInfo* Dragger::rightClickedEventInfo() const
{ return rightclickeventinfo_; }


void Dragger::turnOn( bool yn )
{
    if ( yn )
	onoff_->setAllChildrenOn();
    else
	onoff_->setAllChildrenOff();
}


bool Dragger::isOn() const
{
    return onoff_->getValue( 0 );
}


void Dragger::setSize( const Coord3& size )
{
    osg::Matrix osgmatrix = positiontransform_->getMatrix();
    osgmatrix.makeScale( size.x/2, size.y/2, size.z/2 );
    positiontransform_->setMatrix( osgmatrix );
}


Coord3 Dragger::getSize() const
{
    const osg::Matrix matrix = positiontransform_->getMatrix();
    const osg::Vec3d vec = matrix.getScale();
    return Coord3( vec.x()*2, vec.y()*2, vec.z()*2 );
}


void Dragger::setRotation( const Coord3& vec, float angle )
{
    osg::Matrix osgmatrix = positiontransform_->getMatrix();
    osgmatrix.makeRotate( angle, osg::Vec3(vec.x,vec.y,vec.z));

    positiontransform_->setMatrix( osgmatrix );
}


void Dragger::setPos( const Coord3& pos )
{
    Coord3 newpos = displaytrans_ ? displaytrans_->transform( pos ) : pos;
    
    osg::Matrix osgmatrix = positiontransform_->getMatrix();
    osgmatrix.makeTranslate( newpos.x, newpos.y, newpos.z );
    positiontransform_->setMatrix( osgmatrix );
    
    if ( dragger_ ) dragger_->setMatrix( osg::Matrix::identity() );
}


Coord3 Dragger::getPos() const
{
    osg::Vec3d pos = dragger_->getMatrix().getTrans();
    pos = positiontransform_->getMatrix().preMult( pos );
    const Coord3 coord = Conv::to<Coord3>( pos );
        
    return displaytrans_ ? displaytrans_->transformBack( coord ) : coord;
}


}; // namespace visBase
