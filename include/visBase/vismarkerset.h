#ifndef vismarkerset_h
#define vismarkerset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "trigonometry.h"
#include "draw.h"
#include "viscoord.h"
#include "visnormals.h"

namespace osgGeo { class MarkerSet; }

namespace visBase
{


/*!\brief

MarkerSet is a set of basic pickmarker with a constant size on screen. 
Size and shape are settable.

*/

mExpClass(visBase) MarkerSet : public VisualObjectImpl
{
public:
    static MarkerSet*	create()
			mCreateDataObj(MarkerSet);
    
    Coordinates*	getCoordinates()    { return coords_; }
    Normals*		getNormals();

    void		setMarkerStyle(const MarkerStyle3D&);
    			/*!<Sets predefined shape and size. */
    const MarkerStyle3D& getMarkerStyle() const	{ return markerstyle_; }
    void		setType(MarkerStyle3D::Type);
    			/*!<Sets predefined shape. */
    MarkerStyle3D::Type	getType() const;
 
    void		setScreenSize(const float);
    			/*!<If a nonzero value is given, the object will
			    try to have the same size (in pixels) on the screen
			    at all times. */
    float		getScreenSize() const;
    static float	cDefaultScreenSize() { return 5; }

    void		doFaceCamera(bool yn);
    			/*!<If true, the maker will always be rotated so the
			    same part of the marker always faces the camera. */
    bool		facesCamera() const;

    void		setDisplayTransformation( const mVisTrans* );
    const mVisTrans*	getDisplayTransformation() const;

protected:
				~MarkerSet();
    
    RefMan<Coordinates>		coords_;
    LockedRefMan<Normals>	normals_;
    
    RefMan<const mVisTrans>	displaytrans_;
    osgGeo::MarkerSet*		markerset_;
    MarkerStyle3D		markerstyle_;

    //void		setArrowDir(const ::Sphere&);
};

};


#endif

