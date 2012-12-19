#ifndef vistransform_h
#define vistransform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visdatagroup.h"
#include "position.h"

namespace osg { class MatrixTransform; class Vec3d; class Vec3f; class Quat; }


namespace visBase
{
/*! \brief
The Transformation is an object that transforms everything following the
node.

The transformation is denoted:

Aq=b

Where A is the transformation matrix, q is a column vector with { x, y, z, 1 }
and b is the transformed column vector { x'', y'', z'', m }. The output coords
can be calculated by:

x' = x''/m; y' = y''/m; z'=z''/m;

*/


mClass(visBase) Transformation : public DataObjectGroup
{
public:
    static Transformation*	create()
				mCreateDataObj(Transformation);

    void		reset();

    void		setA(double a11,double a12,double a13,double a14,
	    		     double a21,double a22,double a23,double a24,
			     double a31,double a32,double a33,double a34,
			     double a41,double a42,double a43,double a44 );

    void		setMatrix(const Coord3& trans,
				  const Coord3& rotvec,double rotangle,
				  const Coord3& scale);

    void		setTranslation(const Coord3&);
    void		setRotation(const Coord3& vec,double angle);
    void		setScale(const Coord3&);
    void		setScaleOrientation(const Coord3& vec,double angle);

    Coord3		getTranslation() const;
    Coord3		getScale() const;

    void		setAbsoluteReferenceFrame();

    Coord3		transform(const Coord3&) const;
    Coord3		transformBack(const Coord3&) const;
    void		transform(const Coord3&, osg::Vec3f&) const;
    void		transform(osg::Vec3d&) const;
    void		transformBack(osg::Vec3d&) const;
    void		transform(osg::Vec3f&) const;
    void		transformBack(osg::Vec3f&) const;

    Coord3		transformDir(const Coord3&) const;
    Coord3		transformDirBack(const Coord3&) const;
    
    static void		transform(const Transformation*,const Coord3&,
				  osg::Vec3f&);
    static void		transform(const Transformation*,const osg::Vec3f&,
				  osg::Vec3f&);

private:

    virtual		~Transformation();

    void		updateMatrix();
    void		updateNormalizationMode();

    osg::MatrixTransform* node_;

    osg::Vec3d&		curscale_;
    osg::Vec3d&		curtrans_;
    osg::Quat&		currot_;
    osg::Quat&		curso_;
};

}

#endif

