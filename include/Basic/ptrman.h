#ifndef ptrman_h
#define ptrman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id$
________________________________________________________________________

-*/

#include "general.h"


// We have to make 3 macros because of compiler restrictions
// concerning the total length of macros

#define mDefPtrMan1(Clss, PtrType,PostSet, EraseFunc) \
\
template<class T> \
class Clss \
{ \
public: \
\
			Clss( T* p=0 ) : ptr_( 0 )	{ set(p); } \
			~Clss()		{ erase(); } \
    inline Clss<T>&	operator=( T* p ) \
			{ set(p); return *this; } \
\
    inline T*		ptr()			{ return (T*) ptr_; } \
    inline const T*	ptr() const		{ return (const T*) ptr_; } \
    inline		operator T*()		{ return (T*) ptr_; } \
    inline		operator const T*() const { return (const T*) ptr_; } \
    inline T*		operator ->()		{ return (T*) ptr_; } \
    inline const T*	operator ->() const	{ return (const T*) ptr_; } \
    inline T&		operator *()		{ return (T&)(*ptr_); } \

#define mDefPtrMan2(Clss, PtrType, PostSet, EraseFunc) \
    inline bool			operator !() const { return !ptr_; } \
\
    inline void			erase() \
				{ EraseFunc; ptr_ = 0; } \
    inline void			set( T* p, bool doerase=true ) \
				{ \
				    if ( doerase ) \
					erase(); \
				    ptr_=(PtrType*) p; \
				    PostSet; \
				}

#define mDefPtrMan3(Clss, PtrType, PostSet, EraseFunc) \
private: \
\
    PtrType*				ptr_; \
\
    Clss<T>&			operator=(const T& p) const; \
\
};


/*!\brief a simple autopointer.

It is assigned to a pointer, and takes over
the responsibility for its deletion.
For Arrays, use the ArrPtrMan class.

*/
mDefPtrMan1(PtrMan, T, , delete ptr_ )
inline PtrMan<T>& operator=(const PtrMan<T>& p ); //Will give linkerror is used
mDefPtrMan2(PtrMan, T, , delete ptr_ )
mDefPtrMan3(PtrMan, T, , delete ptr_ )

/*!\brief a simple autopointer for arrays.

For Non-arrays, use the PtrMan class.

*/
mDefPtrMan1(ArrPtrMan, T, , delete [] ptr_)
//Will give linkerror is used
inline ArrPtrMan<T>& operator=(const ArrPtrMan<T>& p );
mDefPtrMan2(ArrPtrMan, T, , delete [] ptr_)
mDefPtrMan3(ArrPtrMan, T, , delete [] ptr_)


#endif
