/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "viscube.h"
#include "vistransform.h"
#include "iopar.h"

#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoTranslation.h>

mCreateFactoryEntry( visBase::Cube );

namespace visBase
{

Cube::Cube()
    : Shape( new SoCube )
    , position( new SoTranslation )
    , transformation( 0 )
{
    //insertNode( position );
}


Cube::~Cube()
{ setDisplayTransformation(0); }


void Cube::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation==nt ) return;

    const Coord3 oldpos = centerPos();
    const Coord3 oldwidth = width();

    if ( transformation )
	transformation->unRef();

    transformation = nt;

    if ( transformation )
	transformation->ref();

    setCenterPos( oldpos );
    setWidth( oldwidth );
}


void Cube::setCenterPos( const Coord3& pos_ )
{
    const Coord3 pos = transformation ? transformation->transform(pos_) : pos_;
    position->translation.setValue( (float) pos.x, 
				    (float) pos.y, (float) pos.z );
}


Coord3 Cube::centerPos() const
{
    Coord3 res;
    SbVec3f pos = position->translation.getValue();

    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];

    return transformation ? transformation->transformBack(res) : res;
}


void Cube::setWidth( const Coord3& n )
{
    pErrMsg("Not impl." );
}


Coord3 Cube::width() const
{
    
    pErrMsg("Not impl." );
    return Coord3::udf();
}


}; //namespace
