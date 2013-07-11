/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vistexturecoords.h"

#include "errh.h"
#include "position.h"
#include "thread.h"

#include <osg/Array>

mCreateFactoryEntry( visBase::TextureCoords );
mCreateFactoryEntry( visBase::TextureCoords2 );

namespace visBase
{

TextureCoords2::TextureCoords2()
    : mutex_( *new Threads::Mutex )
{ 
}


TextureCoords2::~TextureCoords2()
{
}


void TextureCoords2::setCoord( int idx, const Coord& pos )
{
   /* Threads::MutexLocker lock( mutex_ );
    coords_->point.set1Value( idx, (float)pos.x, (float)pos.y );*/
}


SoNode* TextureCoords2::gtInvntrNode()
{ return 0; }


TextureCoords::TextureCoords()
    : mutex_( *new Threads::Mutex )
    , osgcoords_( new osg::Vec2Array )
{
    mGetOsgVec2Arr(osgcoords_)->ref();
}


TextureCoords::~TextureCoords()
{
    delete &mutex_;
    mGetOsgVec2Arr(osgcoords_)->unref();
}


int TextureCoords::size(bool includedeleted) const
{ return  mGetOsgVec2Arr(osgcoords_)->size(); }


void TextureCoords::setCoord( int idx, const Coord3& pos )
{
    Coord crd;
    crd.x = pos.x;
    crd.y = pos.y;
    setCoord( idx, crd );
}


void TextureCoords::setCoord( int idx, const Coord& pos )
{
    Threads::MutexLocker lock( mutex_ );

    if ( idx >=mGetOsgVec2Arr(osgcoords_)->size() )
    {
	osg::Vec2f txcoord;
	txcoord[0] = pos.x;
	txcoord[1] = pos.y;
	mGetOsgVec2Arr(osgcoords_)->push_back( txcoord );
    }
    else
    {
	(*mGetOsgVec2Arr(osgcoords_))[idx][0] = pos.x;
	(*mGetOsgVec2Arr(osgcoords_))[idx][1] = pos.y;
    }
}


int TextureCoords::addCoord( const Coord3& pos )
{
    Coord crd;
    crd.x = pos.x;
    crd.y = pos.y;

    return addCoord( crd );
}


int TextureCoords::addCoord( const Coord& pos )
{
    Threads::MutexLocker lock( mutex_ );
    osg::Vec2f txcoord;
    txcoord[0] = pos.x;
    txcoord[1] = pos.y;
    mGetOsgVec2Arr(osgcoords_)->push_back( txcoord );

    return mGetOsgVec2Arr(osgcoords_)->size() - 1;
}


Coord3 TextureCoords::getCoord( int idx ) const
{
    Threads::MutexLocker lock( mutex_ );

    Coord3 crd( 0.0, 0.0, 0.0 );

    if ( idx < mGetOsgVec2Arr(osgcoords_)->size() )
    {
	osg::Vec2 osgcrd = mGetOsgVec2Arr(osgcoords_)->at( idx );
	crd.x = osgcrd[0];
	crd.y = osgcrd[1];
	crd.z = 0.0f;
    }
    return crd;
}


void TextureCoords::clear()
{
    Threads::MutexLocker lock( mutex_ );
    mGetOsgVec2Arr( osgcoords_ )->clear();
}


int TextureCoords::nextID( int previd ) const
{
 /*   Threads::MutexLocker lock( mutex_ );
    const int sz = coords_->point.getNum();

    int res = previd+1;
    while ( res<sz )
    {
	if ( unusedcoords_.indexOf(res)==-1 )
	    return res;
    }

    return -1;*/
    return - 1;
}



bool TextureCoords::removeCoord(int idx)
{
    Threads::MutexLocker lock( mutex_ );

    if ( idx >=  mGetOsgVec2Arr(osgcoords_)->size() )
	return false;

    mGetOsgVec2Arr(osgcoords_)->erase(
	mGetOsgVec2Arr(osgcoords_)->begin() + idx );
    return true;
}


SoNode* TextureCoords::gtInvntrNode()
{ return 0; }


int  TextureCoords::getFreeIdx()
{
  /*  if ( unusedcoords_.size() )
    {
	const int res = unusedcoords_[unusedcoords_.size()-1];
	unusedcoords_.removeSingle(unusedcoords_.size()-1);
	return res;
    }

    return coords_->point.getNum();*/
    return 0;
}


TextureCoordListAdapter::TextureCoordListAdapter( TextureCoords& c )
    : texturecoords_( c )
    , nrremovedidx_( 0 )
{ texturecoords_.ref(); }


TextureCoordListAdapter::~TextureCoordListAdapter()
{ texturecoords_.unRef(); }

int TextureCoordListAdapter::nextID( int previd ) const
{ return texturecoords_.nextID( previd ); }


int TextureCoordListAdapter::add( const Coord3& p )
{ return texturecoords_.addCoord( p ); }


void TextureCoordListAdapter::addValue( int, const Coord3& p )
{
    pErrMsg("Not implemented");
}


Coord3 TextureCoordListAdapter::get( int idx ) const
{ return texturecoords_.getCoord( idx ); }


bool TextureCoordListAdapter::isDefined( int idx ) const
{ return texturecoords_.getCoord( idx ).isDefined(); }


void TextureCoordListAdapter::set( int idx, const Coord3& p )
{ texturecoords_.setCoord( idx, p ); }


void TextureCoordListAdapter::remove( int idx )
{ texturecoords_.removeCoord( idx ); }


void TextureCoordListAdapter::remove(const TypeSet<int>& idxs)
{
    TypeSet<int> removedidxs;
    for ( int idx = idxs.size()-1; idx--; )
    {
	if ( removedidxs.isPresent( idx ) )
	    continue;
	const int toberemoveidx = 
	    idxs[idx] >= nrremovedidx_ ? idxs[idx] - nrremovedidx_ : idxs[idx];

	if ( texturecoords_.removeCoord( toberemoveidx ) )
	{
	    removedidxs += idxs[idx];
	    nrremovedidx_++;
	}
    }
}


}; // namespace visBase
