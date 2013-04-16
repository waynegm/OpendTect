/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visnormals.h"

#include "errh.h"
#include "trigonometry.h"
#include "thread.h"
#include "vistransform.h"

#include <osg/Array>

mCreateFactoryEntry( visBase::Normals );

namespace visBase
{

Normals::Normals()
    : osgnormals_( new osg::Vec3Array )
    , mutex_( *new Threads::Mutex )
    , transformation_( 0 )
{
    osgnormals_->ref();
}


Normals::~Normals()
{
    delete &mutex_;
    if ( transformation_ ) transformation_->unRef();
    
    osgnormals_->unref();
}


void Normals::setNormal( int idx, const Vector3& n )
{
    osg::Vec3f osgnormal;
    visBase::Transformation::transformDir( transformation_, n, osgnormal );

    Threads::MutexLocker lock( mutex_ );
    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    for ( int idy=osgnormals->size(); idy<idx; idy++ )
    {
	unusednormals_ += idy;
	(*osgnormals)[idy] = osg::Vec3f( mUdf(float),mUdf(float),mUdf(float) );
    }
    osgnormals->push_back( osgnormal );
}


int Normals::nrNormals() const
{ return mGetOsgVec3Arr(osgnormals_)->size(); }


void Normals::clear()
{
    mGetOsgVec3Arr( osgnormals_ )->clear();
}


void Normals::inverse()
{
    Threads::MutexLocker lock( mutex_ );

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    for ( int idx=osgnormals->size()-1; idx>=0; idx-- )
	(*osgnormals)[idx] *= -1;
}


int Normals::nextID( int previd ) const
{
    Threads::MutexLocker lock( mutex_ );

    const int sz = nrNormals();

    int res = previd+1;
    while ( res<sz )
    {
	if ( unusednormals_.indexOf(res)==-1 )
	    return res;
    }

    return -1;
}


int Normals::addNormal( const Vector3& n )
{

    osg::Vec3f osgnormal;
    visBase::Transformation::transformDir( transformation_, n, osgnormal );

    Threads::MutexLocker lock( mutex_ );

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    osgnormals->push_back( osgnormal ) ;

    return nrNormals();
}


void Normals::addNormalValue( int idx, const Vector3& n )
{
    osg::Vec3f osgnormal;
    visBase::Transformation::transformDir( transformation_, n, osgnormal );

    Threads::MutexLocker lock( mutex_ );
    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    if( idx >= nrNormals() )
    {
	osgnormals->push_back( osgnormal ) ;
    }
    else
    {
	float xval = (*osgnormals)[idx][0];
	if( mIsUdf( xval ) )
	    (*osgnormals)[idx] = osgnormal;
	else
	    (*osgnormals)[idx] += osgnormal;

    }

}


void Normals::removeNormal(int idx)
{
    Threads::MutexLocker lock( mutex_ );
    const int nrnormals = nrNormals();

    if ( idx<0 || idx>=nrnormals )
    {
	pErrMsg("Invalid index");
	return;
    }
    
    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );

    if ( idx==nrnormals-1 )
	osgnormals->pop_back();
    else
    {
	unusednormals_ += 1;
	(*osgnormals)[idx] = osg::Vec3f( mUdf(float),mUdf(float),mUdf(float) );
    }
}


void Normals::setAll( const float* vals, int coord3sz )
{
    Threads::MutexLocker lock( mutex_ );

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    if ( coord3sz!=nrNormals() )
	osgnormals->resize( coord3sz );

    int nrnormals( 0 );
    while ( nrnormals< coord3sz )
    {
	( *osgnormals )[nrnormals] = 
	    osg::Vec3f( vals[nrnormals*3], vals[nrnormals*3+1], vals[nrnormals*3+2] );
	nrnormals++;
    }
}


Coord3 Normals::getNormal( int idx ) const
{
    Threads::MutexLocker lock( mutex_ );
    if ( idx>=nrNormals() )
	return Coord3::udf();

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    const osg::Vec3f norm = ( *osgnormals )[idx];
    Coord3 res( norm[0], norm[1], norm[2] );
    transformNormal( transformation_, res, false );

    return res;
}


void Normals::transformNormal( const Transformation* t, Coord3& n,
			       bool todisplay ) const
{
    if ( !t || !n.isDefined() ) return;

    if ( todisplay )
	t->transformBackDir( n );
    else
	t->transformDir( n );
}


void Normals::setDisplayTransformation( const mVisTrans* nt )
{
    if ( nt==transformation_ ) return;

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );

    Threads::MutexLocker lock( mutex_ );
    for ( int idx = 0; idx<nrNormals(); idx++ )
    {
	osg::Vec3f normal;
	visBase::Transformation::transformBackDir( transformation_, 
				 (*osgnormals)[idx], normal );
	visBase::Transformation::transformDir( nt, normal );
	(*osgnormals)[idx] =  normal;
    }

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();
}


}; // namespace visBase
