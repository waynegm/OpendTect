/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visrandomtrack.h"

#include "vistexture2.h"
#include "vistristripset.h"
#include "viscoord.h"
#include "visevent.h"
#include "vistexturecoords.h"
#include "visdataman.h"
#include "viscoltabmod.h"
#include "vistransform.h"
#include "vistexturechannels.h"
#include "iopar.h"
#include "errh.h"

#include <osg/Array>
#include <osgGeo/RandomLine>

mCreateFactoryEntry( visBase::RandomTrack );

namespace visBase
{

RandomTrack::RandomTrack()
    : VisualObjectImpl(false)
    , node_(new osgGeo::TexturePanelStripNode)
    , depthrg_(0,1)
    , knotmovement(this)
    , knotnrchange(this)
    , sectionidx_(-1)
    , transformation_(0)
{
    node_->ref();
    addKnot( Coord( 0, 0 ) );
    addKnot( Coord( 1, 0 ) );
}


RandomTrack::~RandomTrack()
{
    node_->unref();
}


void RandomTrack::setDisplayTransformation( const mVisTrans* tf )
{
    transformation_ = tf;
    rebuild();
}


const mVisTrans* RandomTrack::getDisplayTransformation() const
{
    return transformation_;
}


void RandomTrack::showDragger( bool yn )
{
    pErrMsg("Not impl");
}


bool RandomTrack::isDraggerShown() const
{
    pErrMsg("Not impl");

    return false;
}


void RandomTrack::moveObjectToDraggerPos()
{
}


void RandomTrack::moveDraggerToObjectPos()
{
}


int RandomTrack::nrKnots() const
{ return knots_.size(); }


void RandomTrack::setTrack( const TypeSet<Coord>& posset )
{
    copy( knots_, posset );
    rebuild();
}


void RandomTrack::addKnot( const Coord& pos )
{
    const bool draggershown = isDraggerShown();
    knots_ += pos;
    rebuild();
    showDragger( true );		//Rebuild the dragger
    knotnrchange.trigger();
    if ( !draggershown )
	showDragger( false );
}


void RandomTrack::insertKnot( int idx, const Coord& pos )
{
    const bool draggershown = isDraggerShown();
    knots_.insert( idx, pos );
    rebuild();
    showDragger( true );		//Rebuild the dragger
    knotnrchange.trigger();
    if ( !draggershown )
	showDragger( false );
}


Coord RandomTrack::getKnotPos( int idx ) const
{ return knots_[idx]; }


Coord RandomTrack::getDraggerKnotPos( int idx ) const
{
    return Coord::udf();
}


void RandomTrack::setKnotPos( int idx, const Coord& pos )
{
    if ( idx < nrKnots() )
    {
	knots_[idx] = pos;
	rebuild();
    }
    else
	addKnot( pos );
}


void RandomTrack::setDraggerKnotPos( int idx, const Coord& pos )
{
}


void RandomTrack::removeKnot( int idx )
{
    if ( knots_.size()< 3 )
    {
	pErrMsg("Can't remove knot");
	return;
    }

    knots_.removeSingle( idx );
    rebuild();

    //if ( dragger )
    //dragger->knots.deleteValues( idx, 1 );

    knotnrchange.trigger();
}


void RandomTrack::setDepthInterval( const Interval<float>& drg )
{
    depthrg_ = drg;
    rebuild();
}


const Interval<float> RandomTrack::getDepthInterval() const
{ return depthrg_; }


void RandomTrack::setDraggerDepthInterval( const Interval<float>& intv )
{
}


const Interval<float> RandomTrack::getDraggerDepthInterval() const
{
    return Interval<float>( 0, 1 );
}


#define mSetRange( dim )  \
    createDragger(); \
    SbVec3f xyzstart = dragger->xyzStart.getValue(); \
    SbVec3f xyzstop = dragger->xyzStop.getValue(); \
    SbVec3f xyzstep = dragger->xyzStep.getValue(); \
 \
    xyzstart[dim] = rg.start; \
    xyzstop[dim] = rg.stop; \
    xyzstep[dim] = rg.step; \
 \
    dragger->xyzStart.setValue( xyzstart ); \
    dragger->xyzStop.setValue( xyzstop ); \
    dragger->xyzStep.setValue( xyzstep )

void RandomTrack::setXrange( const StepInterval<float>& rg )
{
    //mSetRange( 0 );
}

void RandomTrack::setYrange( const StepInterval<float>& rg )
{
    //mSetRange( 1 );
}

void RandomTrack::setZrange( const StepInterval<float>& rg )
{
    //mSetRange( 2 );
}


void RandomTrack::setDraggerSize( const Coord3& nz )
{
}


Coord3 RandomTrack::getDraggerSize() const
{
    return Coord3::udf();
}


void RandomTrack::setClipRate( Interval<float> nc )
{
}


void RandomTrack::rebuild()
{
    osg::ref_ptr<osg::Vec2Array> coordarr = new osg::Vec2Array;
    
    for ( int idx=0; idx<knots_.size(); idx++ )
    {
	osg::Vec2f crd;
	if ( transformation_ )
	{
	    osg::Vec3f tmp;
	    transformation_->transform( Coord3(knots_[idx],0), tmp );
	    crd[0] = tmp[0];
	    crd[1] = tmp[1];
	}
	else crd = Conv::to<osg::Vec2f>( knots_[idx] );
	
	coordarr->push_back( crd );
    }
    
    node_->setPath( *coordarr );
}


void RandomTrack::createDragger()
{
}


}; // namespace visBase
