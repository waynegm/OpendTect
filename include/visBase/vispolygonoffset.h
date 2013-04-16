#ifndef vispolygonoffset_h
#define vispolygonoffset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		June 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visnodestate.h"

namespace osg { class PolygonOffset; }

namespace visBase
{
/*!Class that manipulates the zbuffer. See coin for details. */

mExpClass(visBase) PolygonOffset : public NodeState
{
public:
				PolygonOffset();

    void			setFactor(float);
    float			getFactor() const;

    void			setUnits(float);
    float			getUnits() const;

protected:
    				~PolygonOffset();

    osg::PolygonOffset*		offset_;
};

};

#endif

