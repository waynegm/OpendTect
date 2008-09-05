/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : July 2008
-*/

static const char* rcsID = "$Id: polygonsurface.cc,v 1.2 2008-09-05 21:39:31 cvsyuancheng Exp $";

#include "polygonsurface.h"

#include "polygon.h"
#include "trigonometry.h"

namespace Geometry
{


#define mGetValidPolygonIdx( polygonidx, polygonnr, extra, errorres ) \
\
    int polygonidx = polygonnr - firstpolygon_; \
    if ( polygonidx<-extra || polygonidx>polygons_.size()+extra ) \
	return errorres;

#define mGetValidKnotIdx( knotidx, knotnr, polygonidx, extra, errorres ) \
\
    if ( !firstknots_.size() ) return errorres; \
    int knotidx = knotnr - firstknots_[polygonidx]; \
    if ( knotidx<0 && knotidx>polygons_[polygonidx]->size()+extra ) \
	return errorres;

    
PolygonSurface::PolygonSurface()
    : firstpolygon_( 0 )
{}


PolygonSurface::~PolygonSurface()
{
    deepErase( polygons_ );
}


Element* PolygonSurface::clone() const
{
    PolygonSurface* res = new PolygonSurface;
    deepCopy( res->polygons_, polygons_ );
    res->firstknots_ = firstknots_;
    res->firstpolygon_ = firstpolygon_;
    res->polygonnormals_ = polygonnormals_;

    return res;
}


bool PolygonSurface::insertPolygon( const Coord3& firstpos, 
	const Coord3& normal, int polygonnr, int firstknot )
{
    if ( !firstpos.isDefined() || !normal.isDefined() )
	return false;

    if ( polygons_.isEmpty() )
	firstpolygon_ = polygonnr;

    mGetValidPolygonIdx( polygonidx, polygonnr, 1, false );
    if ( polygonidx==-1 )
    {
	firstpolygon_--;
	polygonidx++;
    }

    if ( polygonidx==polygons_.size() )
    {
	polygons_ += new TypeSet<Coord3>;
	polygonnormals_ += normal;
	firstknots_ += firstknot;
    }
    else
    {
	polygons_.insertAt( new TypeSet<Coord3>, polygonidx );
	polygonnormals_.insert( polygonidx, normal );
	firstknots_.insert( polygonidx,firstknot );
    }

    polygons_[polygonidx]->insert( 0, firstpos );

    triggerNrPosCh( RowCol(polygonidx,PolygonInsert).getSerialized() );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );

    return true;
}


bool PolygonSurface::removePolygon( int polygonnr )
{
    mGetValidPolygonIdx( polygonidx, polygonnr, 0, false );

    polygons_.remove( polygonidx );
    polygonnormals_.remove( polygonidx );
    firstknots_.remove( polygonidx );

    triggerNrPosCh( RowCol(polygonidx,PolygonRemove).getSerialized() );
    if ( blocksCallBacks() )
	blockCallBacks( true, true );
    
    return true;
}


bool PolygonSurface::insertKnot( const RCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return false;

    mGetValidPolygonIdx( polygonidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), polygonidx, 1, false );
    if ( knotidx==-1 )
    {
	firstknots_[polygonidx]--;
	knotidx++;
    }

    const int nrknots = polygons_[polygonidx]->size();
    if ( nrknots==3 )
    {
	polygonnormals_[polygonidx] = ( ((*polygons_[polygonidx])[1]-
		(*polygons_[polygonidx])[0]).cross( (*polygons_[polygonidx])[2]-
		(*polygons_[polygonidx])[1]) ).normalize();
    }

    if ( knotidx==nrknots )
	(*polygons_[polygonidx]) += pos;
    else
	polygons_[polygonidx]->insert( knotidx, pos );

    triggerNrPosCh( RowCol(polygonidx,PolygonChange).getSerialized() );

    return true;
}


bool PolygonSurface::removeKnot( const RCol& rc )
{
    mGetValidPolygonIdx( polygonidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), polygonidx, 0, false );

    if ( polygons_[polygonidx]->size() <= 1 )
	return removePolygon( rc.r() );

    polygons_[polygonidx]->remove( knotidx );
    triggerNrPosCh( RowCol(polygonidx,PolygonChange).getSerialized() );
    
    return true;
}


#define mEmptyInterval() StepInterval<int>( mUdf(int), mUdf(int), mUdf(int) )

StepInterval<int> PolygonSurface::rowRange() const
{
    if ( polygons_.isEmpty() )
	return mEmptyInterval();
    
    return StepInterval<int>(firstpolygon_,firstpolygon_+polygons_.size()-1,1); 
}


StepInterval<int> PolygonSurface::colRange( int polygonnr ) const
{
    mGetValidPolygonIdx( polygonidx, polygonnr, 0, mEmptyInterval() );

    const int firstknot = firstknots_[polygonidx];
    return StepInterval<int>( firstknot,
	    firstknot+polygons_[polygonidx]->size()-1, 1 );
}


bool PolygonSurface::getPolygonCrds( int polygonnr, TypeSet<Coord3>& pts ) const
{
    mGetValidPolygonIdx( polygonidx, polygonnr, 0, false );
    for ( int idx=0; idx<(*polygons_[polygonidx]).size(); idx++ )
    	pts += (*polygons_[polygonidx])[idx];

    return pts.size();
}


bool PolygonSurface::getSurfaceCrds( TypeSet<Coord3>& pts ) const
{
    for ( int polygonidx=0; polygonidx<polygons_.size(); polygonidx++ )
    {
    	for ( int idx=0; idx<(*polygons_[polygonidx]).size(); idx++ )
    	    pts += (*polygons_[polygonidx])[idx];
    }

    return pts.size();
}


bool PolygonSurface::setKnot( const RCol& rc, const Coord3& pos )
{
    if ( !pos.isDefined() )
	return removeKnot( rc );

    mGetValidPolygonIdx( polygonidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), polygonidx, 0, false );

    //Make sure no selfintersecting when we drag points around.
    const Coord3 oldpos = (*polygons_[polygonidx])[knotidx];
    const int nrknots = (*polygons_[polygonidx]).size();
    for ( int idx=0; idx<nrknots; idx++ )
    {
	const int nextidx = idx<nrknots-1 ? idx+1 : 0;
	if ( idx==knotidx || nextidx==knotidx )
	    continue;

	const Coord3 v0 = (*polygons_[polygonidx])[idx];
	const Coord3 v1 = (*polygons_[polygonidx])[nextidx];
	if ( !sameSide3D( pos, oldpos, v0, v1, 0 ) )
	    return false;
    }
   
    (*polygons_[polygonidx])[knotidx] = pos;
    triggerMovement( RowCol(polygonidx,PolygonChange).getSerialized() );
    return true;
}


Coord3 PolygonSurface::getKnot( const RCol& rc ) const
{
    mGetValidPolygonIdx( polygonidx, rc.r(), 0, Coord3::udf() );
    mGetValidKnotIdx( knotidx, rc.c(), polygonidx, 0, Coord3::udf() );
    
    return (*polygons_[polygonidx])[knotidx];
}


bool PolygonSurface::isKnotDefined( const RCol& rc ) const
{
    mGetValidPolygonIdx( polygonidx, rc.r(), 0, false );
    mGetValidKnotIdx( knotidx, rc.c(), polygonidx, 0, false );

    return true;
}


const Coord3& PolygonSurface::getPolygonNormal( int polygon ) const
{
    mGetValidPolygonIdx( polygonidx, polygon, 0, Coord3::udf() );
    if ( polygonidx < polygonnormals_.size() )
	return  polygonnormals_[polygonidx];

    return Coord3::udf();
}


void PolygonSurface::getPolygonConcaveTriangles( int polygon, 
						 TypeSet<int>& triangles ) const
{
    const StepInterval<int> colrg = colRange( polygon );
    if ( colrg.isUdf() || colrg.nrSteps()<3 )
	return;

    const Coord3 normal = getPolygonNormal( polygon );

    for ( int idx=colrg.start; idx<=colrg.stop; idx += colrg.step )
    {
    	for ( int idy=idx+colrg.step; idy<=colrg.stop; idy += colrg.step )
    	{
	    for ( int idz=idy+colrg.step; idz<=colrg.stop; idz += colrg.step )
	    {
		const Coord3 v0 = getKnot( RowCol(polygon,idx) );
		const Coord3 v1 = getKnot( RowCol(polygon,idy) );
		const Coord3 v2 = getKnot( RowCol(polygon,idz) );
		const Coord3 trinormal = ( (v1-v0).cross(v2-v1) ).normalize();
		if ( normal.dot(trinormal)<0 )  
		{
		    triangles += idx;
		    triangles += idy;
		    triangles += idz;
		}
	    }
	}
    }
}


void PolygonSurface::getNonintersectConcaveTris( int polygon, 
						 TypeSet<int>& triangles ) const
{
    const StepInterval<int> colrg = colRange( polygon );
    if ( colrg.isUdf() || colrg.nrSteps()<3 )
	return;

    const Coord3 normal = getPolygonNormal( polygon );

    for ( int idx=colrg.start; idx<colrg.stop; idx += colrg.step )
    {
	const Coord3 v0 = getKnot( RowCol(polygon,idx) );
	const Coord3 v1 = getKnot( RowCol(polygon,idx+colrg.step) );

    	for ( int idy=idx+2*colrg.step; idy<=colrg.stop; idy += colrg.step )
    	{
	    const Coord3 v2 = getKnot( RowCol(polygon,idy) );
	    const Coord3 trinormal = ( (v1-v0).cross(v2-v1) ).normalize();
	    if ( normal.dot(trinormal)>0 )
	      continue;
		
	    bool intersecting = false;
	    for ( int idz=idx+colrg.step; idz<idy; idz += colrg.step )
	    {
		const Coord3 p0 = getKnot( RowCol(polygon,idz) );
		const Coord3 p1 = getKnot( RowCol(polygon,idz+colrg.step) );
		if ( linesegmentsIntersecting( v0, v1, p0, p1 ) )
		{
		    intersecting = true;
		    break;
		}
	    }

	    if ( !intersecting )
	    {
		triangles += idx;
		triangles += idx+colrg.step;
		triangles += idy;
	    }
	}
    }

    const Coord3 v0 = getKnot( RowCol(polygon,colrg.start) );
    const Coord3 v1 = getKnot( RowCol(polygon,colrg.stop) );
    for ( int idy=colrg.start+colrg.step; idy<colrg.stop; idy += colrg.step )
    {
	const Coord3 v2 = getKnot( RowCol(polygon,idy) );
	const Coord3 trinormal = ( (v2-v0).cross(v1-v2) ).normalize();
	if ( normal.dot(trinormal)>0 )
	    continue;
	
	bool intersecting = false;
	for ( int idz=colrg.start+colrg.step; idz<idy; idz += colrg.step )
	{
	    const Coord3 p0 = getKnot( RowCol(polygon,idz) );
	    const Coord3 p1 = getKnot( RowCol(polygon,idz+colrg.step) );
	    if ( linesegmentsIntersecting( v0, v1, p0, p1 ) )
	    {
		intersecting = true;
		break;
	    }
	}
	
	if ( !intersecting )
	{
	    triangles += colrg.start;
	    triangles += idy;
	    triangles += colrg.stop;
	}
    }
}


bool PolygonSurface::linesegmentsIntersecting( const Coord3& v0,  
	const Coord3& v1,  const Coord3& p0,  const Coord3& p1 ) const
{
    const Coord3 norm0 = (v1-v0).cross(p0-v0);
    const Coord3 norm1 = (v1-v0).cross(p1-v0);
    if ( norm0.dot(norm1)>0 )
	return false;

    Line3 segment0( v0, v1 );
    Line3 segment1( p0, p1 );
    double t0, t1;
    if ( segment0.closestPoint( segment1, t0, t1 ) && (t0>0 && t0<1) && 
	 (t1>0 && t1<1) )
	return true;

    return false;
}


void PolygonSurface::addEditPlaneNormal( const Coord3& normal )
{
    polygonnormals_ += normal; 
}


void PolygonSurface::addUdfPolygon( int polygonnr, int firstknotnr, int nrknots)
{
    if ( isEmpty() )
	firstpolygon_ = polygonnr;

    firstknots_ += firstknotnr;    
    polygons_ += new TypeSet<Coord3>( nrknots, Coord3::udf() );
}


} // namespace Geometry
