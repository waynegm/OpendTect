/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: rowcolsurface.cc,v 1.1 2006-04-18 18:04:34 cvskris Exp $";

#include "rowcolsurface.h"


namespace Geometry
{


void RowColSurface::getPosIDs( TypeSet<GeomPosID>& pids, bool remudf ) const
{
    pids.erase();
    const StepInterval<int> rowrg = rowRange();
    if ( rowrg.start>rowrg.stop )
	return;

    const int nrrows = rowrg.nrSteps()+1;

    RowCol rc;
    for ( int rowidx=0; rowidx<nrrows; rowidx++ )
    {
	rc.row = rowrg.atIndex( rowidx );
	const StepInterval<int> colrg = colRange( rc.row );
	if ( colrg.start>colrg.stop )
	    continue;

	const int nrcols = colrg.nrSteps()+1;

	for ( int colidx=0; colidx<nrcols; colidx++ )
	{
	    rc.col = colrg.atIndex( colidx );

	    if ( remudf && !isKnotDefined(rc) ) continue;

	    pids += rc.getSerialized();
	}
    }
}


StepInterval<int> RowColSurface::colRange() const
{
    StepInterval<int> res( INT_MAX, INT_MIN, 1 );

    const StepInterval<int> rowrg = rowRange();
    if ( rowrg.start>rowrg.stop )
	return res;

    const int nrrows = rowrg.nrSteps()+1;

    for ( int rowidx=0; rowidx<nrrows; rowidx++ )
    {
	const int row = rowrg.atIndex( rowidx );
	const StepInterval<int> colrg = colRange( row );
	if ( colrg.start>colrg.stop )
	    continue;

	res.include( colrg.start );
	res.include( colrg.stop );
	res.step = colrg.step;
    }

    return res;
}


Coord3 RowColSurface::getPosition( GeomPosID pid ) const
{ return getKnot( RowCol(pid) ); }


bool RowColSurface::setPosition( GeomPosID pid, const Coord3& pos )
{ return setKnot( RowCol(pid), pos ); }


bool RowColSurface::isDefined( GeomPosID pid ) const
{ return isKnotDefined( RowCol(pid) ); }

}; //namespace

