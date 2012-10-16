/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visfaceset.h"

mCreateFactoryEntry( visBase::FaceSet );

namespace visBase
{

FaceSet::FaceSet()
{
    pErrMsg("Should never be called with osg.");
}

}; // namespace visBase
