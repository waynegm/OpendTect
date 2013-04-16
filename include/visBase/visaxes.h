#ifndef visaxes_h
#define visaxes_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          January 2013
 RCS:           $Id$
________________________________________________________________________

-*/


#include "visdata.h"


namespace osg{ class Geode; }
namespace osgGeo{ class AxesNode; }

namespace visBase
{

    
mExpClass(visBase) Axes : public DataObject
{
public:
    static Axes*		create()
    				mCreateDataObj(Axes);
    void			setRadius(float);
    float			getRadius() const;	

protected:
    				~Axes();

    osgGeo::AxesNode*		axesnode_;
    bool			ison_;
};

} // namespace visBase
#endif
