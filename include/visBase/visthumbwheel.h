#ifndef visthumbwheel_h
#define visthumbwheel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          October 2012
 RCS:           $Id: visdragger.h 26837 2012-10-17 09:36:11Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "visdragger.h"

namespace osg { class Group; class MatrixTransform; }
namespace osgGeo { class ThumbWheel; }


namespace visBase
{

/*! \brief Class for simple draggers
*/

class Transformation;
    
    
mClass(visBase) ThumbWheel : public DraggerBase
{
public:
    static ThumbWheel*		create()
    				mCreateDataObj(ThumbWheel);

    void			getAngle() const;
protected:
    					~ThumbWheel();
    
    osg::Node*				gtOsgNode();
    
    OsgRefMan<osg::Group>		onoff_;
    OsgRefMan<osgGeo::ThumbWheel>	thumbwheel_;
    OsgRefMan<osg::MatrixTransform>	positiontransform_;
};

} // namespace visBase

#endif

