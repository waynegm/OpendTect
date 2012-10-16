#ifndef visosg_h
#define visosg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Sep 2012
 RCS:		$Id$
________________________________________________________________________


-*/

/*! Definition of macros used to make osg-life easier */

#include "refcount.h"
#include "visbasemod.h"

class Coord3;

namespace osg { class Vec3f; class Array; }
    

#define mGetOsgArrPtr(tp,ptr) ((tp) ptr->getDataPointer() )
#define mGetOsgVec2Arr(ptr) ((osg::Vec2Array*) ptr )
#define mGetOsgVec3Arr(ptr) ((osg::Vec3Array*) ptr )
#define mGetOsgVec4Arr(ptr) ((osg::Vec4Array*) ptr )


namespace  visBase
{

//!Calls obj->ref(). obj must inherit osg::Referenced
mExternC(visBase) void refOsgObj(void* obj);

//!Calls obj->unref(). obj must inherit osg::Referenced
mExternC(visBase) void unrefOsgObj(void*);

mDefRefMan( OsgRefMan, refOsgObj(ptr_), unrefOsgObj(ptr_) )
} //Namespace

#if defined(visBase_EXPORTS) || defined(VISBASE_EXPORTS)
//Only available in visBase
#include <osg/Vec3>
#include <osg/Vec3d>
#include <position.h>
#include <convert.h>
    
namespace Conv
{
    template <>
    inline void set( Coord3& _to, const osg::Vec3f& v )
    { _to.x = v[0]; _to.y=v[1]; _to.z=v[2]; }
    
    template <>
    inline void set( osg::Vec3f& _to, const Coord3& v )
    { _to.set( (float) v.x, (float) v.y, (float) v.z ); }
    
    template <>
    inline void set( Coord3& _to, const osg::Vec3d& v )
    { _to.x = v[0]; _to.y=v[1]; _to.z=v[2]; }
    
    template <>
    inline void set( osg::Vec3d& _to, const Coord3& v )
    { _to.set(  v.x, v.y, v.z ); }
    
} //Namespace conv

namespace Values
{
    template<>
    class Undef<osg::Vec3f>
    {
    public:
	static void		setUdf( osg::Vec3f& i )	{}
    };
    
    template<>
    class Undef<osg::Vec3d>
    {
    public:
	static void		setUdf( osg::Vec3d& i )	{}
    };

} //Namespace Values

#endif

#endif

