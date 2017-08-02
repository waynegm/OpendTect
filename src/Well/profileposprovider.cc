/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : April 2017
-*/

#include "profileposprovider.h"
#include "profilebase.h"
#include "polylinend.h"


ProfilePosProviderFromLine::ProfilePosProviderFromLine(
	const TypeSet<Coord>& positions )
    : linepositions_(positions)
{
    linegeom_ = new PolyLineND<Coord>( positions );
    arclength_ = linegeom_->arcLength();
}


ProfilePosProviderFromLine::~ProfilePosProviderFromLine()
{
    delete linegeom_;
}


Coord ProfilePosProviderFromLine::getCoord( float pos ) const
{
    return linegeom_->getPoint( pos * arclength_ );
}


bool ProfilePosProviderFromWellPos::isOK( const ProfileModelBase& pm )
{
    const int nruniqwells = pm.nrWells( true );
    if ( nruniqwells < 1 ) return false;
    if ( nruniqwells > 1 ) return true;

    // only if one single well is used multiple times we can do nothing useful
    const int nrwells = pm.nrWells( false );
    return nrwells == 1;
}


Coord ProfilePosProviderFromWellPos::getCoord( float pos ) const
{
    if ( !mIsUdf(sectionangle_) && !mIsUdf(sectionlength_) )
    {
	const double sinang = sin( sectionangle_ );
	const double cosang = cos( sectionangle_ );
	const int firstwellidx = model_.wellIndexAfter( -1.0f );
	if ( firstwellidx<0 )
	{
	    pErrMsg( "Well not found in profile" );
	    return Coord::udf();
	}

	const ProfileBase* wellporf = model_.get( firstwellidx );
	const double dist = sectionlength_ * (pos - wellporf->pos_);
	const Coord& wcoord = wellporf->coord_;
	return Coord( wcoord.x + dist*cosang, wcoord.y + dist*sinang );
    }

    int prevposidx = model_.wellIndexBefore( pos );
    int afterposidx = model_.wellIndexAfter( pos );
    if ( prevposidx==-1 && afterposidx==-1 )
    {
	pErrMsg( "No positions found to get coordinate" );
	return Coord::udf();
    }

    if ( prevposidx<0 )
    {
	const float afterpos = model_.get( afterposidx )->pos_;
	prevposidx = afterposidx;
	afterposidx = model_.wellIndexAfter( afterpos );
	if ( afterposidx<0 )
	{
	    pErrMsg( "Atleast 2 positions needed to get coordinate" );
	    return Coord::udf();
	}
    }
    else if ( afterposidx<0 )
    {
	const float prevpos = model_.get( prevposidx )->pos_;
	afterposidx = prevposidx;
	prevposidx = model_.wellIndexBefore( prevpos );
	if ( prevposidx<0 )
	{
	    pErrMsg( "Atleast 2 positions needed to get coordinate" );
	    return Coord::udf();
	}
    }

    const ProfileBase* prevprof = model_.get( prevposidx );
    const ProfileBase* afterprof = model_.get( afterposidx );
    const float nrdists = (pos - prevprof->pos_) /
			  (afterprof->pos_ - prevprof->pos_);
    const Coord& c0 = prevprof->coord_; const Coord& c1 = afterprof->coord_;
    const Coord delta( c1.x - c0.x, c1.y - c0.y );
    return Coord( c0.x + delta.x * nrdists, c0.y + delta.y * nrdists );
}


