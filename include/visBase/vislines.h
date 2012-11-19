#ifndef vislines_h
#define vislines_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		18-10-2012
 RCS:		$Id: vislines.h 26803 2012-10-16 13:01:04Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________


-*/


#include "visshape.h"


namespace visBase
{

mClass(visBase) Lines : public VertexShape
{
public:
    static Lines*		create()
				mCreateDataObj(Lines);
};

} //namspace visBase

#endif
