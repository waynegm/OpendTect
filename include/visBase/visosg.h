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

#define mGetOsgArrPtr(tp,ptr) ((tp) ptr->getDataPointer() )
#define mGetOsgVec2Arr(ptr) ((osg::Vec2Array*) ptr )
#define mGetOsgVec3Arr(ptr) ((osg::Vec3Array*) ptr )
#define mGetOsgVec4Arr(ptr) ((osg::Vec4Array*) ptr )


//!Calls obj->ref(). obj must inherit osg::Referenced
mExternC(visBase) void refOsgObj(void* obj);

//!Calls obj->unref(). obj must inherit osg::Referenced
mExternC(visBase) void unrefOsgObj(void*);

mDefRefMan( OsgRefMan, refOsgObj(ptr_), unrefOsgObj(ptr_) )

#endif

