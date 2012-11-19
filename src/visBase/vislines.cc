/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          October 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: vislines.cc 26839 2012-10-17 09:38:56Z kristofer.tingdahl@dgbes.com $";

#include "vislines.h"

mCreateFactoryEntry( visBase::Lines );

namespace visBase
{

Lines::Lines()
    : VertexShape(Geometry::PrimitiveSet::Lines,true)
{}

} // namspace visBase
