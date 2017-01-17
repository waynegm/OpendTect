/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "profilesetcreator.h"
#include "profilebase.h"
#include "array1dinterpol.h"
#include "arrayndimpl.h"
#include "polylinend.h"
#include "sorting.h"
#include "zaxistransform.h"
#include "zvalueprovider.h"
#include "survinfo.h"
#include "ioman.h"
#include "ioobj.h"
#include <math.h>


class ProfilePosData
{
public:
		ProfilePosData( ProfileBase* p, float pos,
				Coord coord=Coord(mUdf(float),0) )
		    : prof_(p)
		    , pos_(pos)
		    , coord_(coord)
		    , zhor_(mUdf(float))
		    , zoffs_(mUdf(float))		{}
    bool	operator ==( const ProfilePosData& oth ) const
					{ return prof_ == oth.prof_; }
    bool	isWell() const		{ return prof_ && prof_->isWell(); }
    bool	hasCoord() const	{ return !mIsUdf(coord_.x); }
    bool	hasHorZ() const		{ return !mIsUdf(zhor_); }
    bool	hasZOffset() const	{ return !mIsUdf(zoffs_); }

    ProfileBase*	prof_;
    float		pos_;
    Coord		coord_;
    float		zhor_;
    float		zoffs_;

};


class ProfilePosDataSet : public TypeSet<ProfilePosData>
{
public:

ProfilePosDataSet( ProfileSet& pms )
    : profs_(pms)
{
}

void addExistingProfiles()
{
    for ( int iprof=0; iprof<profs_.size(); iprof++ )
    {
	ProfileBase* prof = profs_.get( iprof );
	if ( prof->isWell() )
	    *this += ProfilePosData( prof, prof->pos_, prof->coord_ );
	else
	    *this += ProfilePosData( prof, prof->pos_ );
    }
}

int nrProfiles() const
{
    int nrp = 0;
    for ( int ippd=0; ippd<size(); ippd++ )
	if ( (*this)[ippd].prof_ ) nrp++;
    return nrp;
}

void setAngCoord( int ippd, int iwellppd, float ang, float len )
{
    ProfilePosData& ppd = (*this)[ippd];
    const ProfilePosData& wellppd = (*this)[iwellppd];
    const double sinang = sin( ang );
    const double cosang = cos( ang );
    const double dist = len * (ppd.pos_ - wellppd.pos_);
    const Coord& wcoord = wellppd.coord_;
    ppd.coord_ = Coord( wcoord.x + dist*cosang, wcoord.y + dist*sinang );
}

void setInterpCoord( int ippd, int ippd0, int ippd1 )
{
    ProfilePosData& ppd = (*this)[ippd];
    const ProfilePosData& ppd0 = (*this)[ippd0];
    const ProfilePosData& ppd1 = (*this)[ippd1];
    const float nrdists = (ppd.pos_ - ppd0.pos_) / (ppd1.pos_ - ppd0.pos_);
    const Coord& c0 = ppd0.coord_; const Coord& c1 = ppd1.coord_;
    const Coord delta( c1.x - c0.x, c1.y - c0.y );
    ppd.coord_ = Coord( c0.x + delta.x * nrdists, c0.y + delta.y * nrdists );
}

    ProfileSet& profs_;

};


#define mErrRet(s) { errmsg_ = s; return false; }


ProfileSetFromEventCreator::ProfileSetFromEventCreator( ProfileSet& p, int mxp )
    : profs_(p)
    , ppds_(*new ProfilePosDataSet(p))
    , t2dtr_(0)
    , t2dpar_(*new IOPar)
    , maxnrprofs_(mxp>0 ? mxp : 50)
    , movepol_(MoveAll)
    , needt2d_(SI().zIsTime()) // just def
{
    for ( int idx=0; idx<profs_.size(); idx++ )
    {
	ProfileBase* prof = profs_.get( idx );
	if ( prof->isWell() )
	{
	    t2dpar_.set( sKey::ID(), prof->wellid_ );
	    t2dpar_.set( sKey::Name(), "WellT2D" );
	    break;
	}
    }
}


ProfileSetFromEventCreator::~ProfileSetFromEventCreator()
{
    reset();
    delete &ppds_;
    delete &t2dpar_;
}


void ProfileSetFromEventCreator::reset()
{
    if ( t2dtr_ )
	{ t2dtr_->unRef(); t2dtr_ = 0; }
    t2dpar_.setEmpty();
}


bool ProfileSetFromEventCreator::go( TaskRunner* tr )
{
    errmsg_.setEmpty();
    if ( ppds_.isEmpty() )
	return true;

    t2dtr_ = ZAxisTransform::create( t2dpar_ );
    if ( t2dtr_ && t2dtr_->needsVolumeOfInterest() )
    {
	TrcKeyZSampling cs;
	for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	{
	    const BinID bid( SI().transform(ppds_[ippd].coord_) );
	    if ( ippd )
		cs.hsamp_.include( bid );
	    else
	    {
		cs.hsamp_.start_.inl() = cs.hsamp_.stop_.inl() = bid.inl();
		cs.hsamp_.start_.crl() = cs.hsamp_.stop_.crl() = bid.crl();
	    }
	}
	const int voiidx = t2dtr_->addVolumeOfInterest( cs, true );
	t2dtr_->loadDataIfMissing( voiidx, tr );
    }

    if ( doGo( tr ) )
    {
	profs_.removeAtSamePos();
	return true;
    }

    return false;
}


void ProfileSetFromEventCreator::addNewPositions()
{
    const int oldsz = ppds_.size();
    const int nrnew = maxnrprofs_ - oldsz;
    if ( nrnew < 1 )
	return;

    // Need to distribute nrnew profiles over 'bins' that are unequal of size
    // 1. give each bin how many to add per bin in int, remember rests
    // 2. give the remaining ones to the largest rests

    // The following sets have sizes oldsz + 1: the number of segments to fill
    TypeSet<int> nr2add; TypeSet<float> fnrleft; TypeSet<int> idxs;
    int nrstill2bdone = nrnew;
    for ( int ippd=0; ippd<=oldsz; ippd++ )
    {
	float prevpos = ippd ? ppds_[ippd-1].pos_ : 0.f;
	float nextpos = ippd < oldsz ? ppds_[ippd].pos_ : 1.f;
	const float dpos = nextpos - prevpos;
	const float fnr2add = dpos * nrnew;
	const int nr2addnow = (int)fnr2add;
	nr2add += nr2addnow;
	fnrleft += fnr2add - nr2addnow;
	idxs += idxs.size();
	nrstill2bdone -= nr2addnow;
    }

    if ( nrstill2bdone > 0 )
    {
	sort_coupled( fnrleft.arr(), idxs.arr(), fnrleft.size() );
	// sorts ascending, thus need to start at end
	const int lastidx = idxs.size() - 1;
	for ( int idx=0; idx<nrstill2bdone; idx++ )
	    nr2add[ idxs[lastidx-idx] ] += 1;
    }

    int ippd = -1;
    for ( int idx=0; idx<=oldsz; idx++ )
	ippd = addNewProfilesAfter( ippd, nr2add[idx], idx == oldsz );
}


int ProfileSetFromEventCreator::addNewProfilesAfter( int ippd,
					int nr2add, bool islast )
{
    if ( nr2add < 1 ) return ippd + 1;
    const int nrppds = ppds_.size();

    const int ippdbefore = ippd > -1 ? ippd : 0;
    const int ippdafter = ippd < nrppds-1 ? ippd+1 : nrppds - 1;
    const ProfilePosData ppd0 = ppds_[ippdbefore];
    const ProfilePosData ppd1 = ppds_[ippdafter];
    const ProfileBase* p0 = ppd0.prof_;
    const ProfileBase* p1 = ppd1.prof_;
    if ( !p0 || !p1 ) { pErrMsg("Huh"); return ippd + 1; }
    const float pos0 = ippd > -1 ? p0->pos_ : 0.f;
    const float pos1 = islast ? 1.f : p1->pos_;

    const float dpos = (pos1 - pos0) / (nr2add+1);
    int inewppd = ippd + 1;
    for ( int iadd=0; iadd<nr2add; iadd++ )
    {
	const float newpos = pos0 + (iadd+1) * dpos;
	ProfileBase* newprof = ProfFac().create( newpos );
	newprof->createMarkersFrom( p0, p1 );
	profs_.set( newprof, false );
	ppds_.insert( inewppd, ProfilePosData(newprof,newpos) );
	inewppd++;
    }

    return inewppd;
}


void ProfileSetFromEventCreator::getKnownDepths(
	const ZValueProvider& zprov, const char* mrkrnm )
{
    // Get the depths from the horizon + transform
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& ppd = ppds_[ippd];
	ppd.zhor_ = zprov.getZValue( ppd.coord_ );
	if ( !ppd.hasHorZ() )
	    continue;

	if ( needt2d_ )
	{
	    if ( !t2dtr_ )
		{ pErrMsg("No T2D"); ppd.zhor_ *= 2000; }
	    else
		ppd.zhor_ = t2dtr_->transform( Coord3(ppd.coord_,ppd.zhor_) );
	}

	if ( ppd.isWell() )
	{
	    const Well::Marker* mrkr = ppd.prof_->markers_.getByName( mrkrnm );
	    ppd.zoffs_ = mrkr ? mrkr->dah() - ppd.zhor_ : mUdf(float);
	}
    }

    Array1DImpl<float> zvals( ppds_.size() );
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	zvals.set( ippd, ppds_[ippd].zhor_ );
    LinearArray1DInterpol zvalinterp;
    zvalinterp.setExtrapol( true );
    zvalinterp.setArray( zvals );
    zvalinterp.execute();
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	ppds_[ippd].zhor_ = zvals.get( ippd );
}


void ProfileSetFromEventCreator::interpolateZOffsets()
{
    int iprevppd = -1;
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	if ( ppds_[ippd].hasZOffset() )
	    { iprevppd = ippd; break; }
    }
    if ( iprevppd < 0 )
    {
	for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	    ppds_[ippd].zoffs_ = 0;
	return;
    }

    // set offs before first defined
    for ( int ippd=0; ippd<iprevppd; ippd++ )
	ppds_[ippd].zoffs_ = ppds_[iprevppd].zoffs_;

    // set undef offs in center part
    for ( int ippd=iprevppd+1; ippd<ppds_.size(); ippd++ )
    {
	if ( !ppds_[ippd].hasZOffset() )
	    continue;

	const float posdist = ppds_[ippd].pos_ - ppds_[iprevppd].pos_;
	for ( int ippdinterp=iprevppd+1; ippdinterp<ippd; ippdinterp++ )
	{
	    const float relpos = (ppds_[ippdinterp].pos_-ppds_[iprevppd].pos_)
				/ posdist;
	    ppds_[ippdinterp].zoffs_ = relpos	      * ppds_[ippd].zoffs_
				     + (1.f - relpos) * ppds_[iprevppd].zoffs_;
	}
	iprevppd = ippd;
    }

    // set offs after last defined
    for ( int ippd=iprevppd+1; ippd<ppds_.size(); ippd++ )
	ppds_[ippd].zoffs_ = ppds_[iprevppd].zoffs_;
}


void ProfileSetFromEventCreator::setMarkerDepths( const char* mrknm )
{
    interpolateZOffsets();

    if ( ppds_.isEmpty() || !ppds_[0].hasHorZ() )
	return;

    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& ppd = ppds_[ippd];
	if ( !ppd.prof_ || ppd.isWell() )
	    continue;

	const int midx = ppd.prof_->markers_.indexOf( mrknm );
	if ( midx >= 0 )
	{
	    const float orgz = ppd.prof_->markers_[midx]->dah();
	    const float newz = ppd.zhor_ + ppd.zoffs_;
	    ppd.prof_->moveMarker( midx, newz,
				   doPush(orgz,newz), doPull(orgz,newz) );
	}
    }
}


bool ProfileSetFromEventCreator::doPush( float orgz, float newz ) const
{
    if ( movepol_ < MoveAbove )
	return movepol_ == MoveAll;
    return (orgz < newz && movepol_ == MoveBelow)
	|| (orgz > newz && movepol_ == MoveAbove);
}


bool ProfileSetFromEventCreator::doPull( float orgz, float newz ) const
{
    if ( movepol_ < MoveAbove )
	return movepol_ == MoveAll;
    return (orgz > newz && movepol_ == MoveBelow)
	|| (orgz < newz && movepol_ == MoveAbove);
}



ProfileSetFromSingleEventCreator::ProfileSetFromSingleEventCreator(
	ProfileSet& m)
    : ProfileSetFromEventCreator(m)
    , zvalprov_(0)
    , sectionangle_(0)
    , sectionlength_(1000)
{
}


void ProfileSetFromSingleEventCreator::init()
{
    ppds_.addExistingProfiles();
    addNewPositions();
    fillCoords();
}


void ProfileSetFromSingleEventCreator::reset()
{
    zvalprov_ = 0;
    ProfileSetFromEventCreator::reset();
}


void ProfileSetFromSingleEventCreator::fillCoords()
{
    int icoordprof0 = -1, icoordprof1 = -1;
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	if ( ppds_[ippd].hasCoord() )
	{
	    if ( icoordprof0 < 0 )
		icoordprof0 = ippd;
	    else if ( icoordprof1 < 0 )
		{ icoordprof1 = ippd; break; }
	}
    }

    if ( icoordprof0 < 0 )
	return;
    if ( icoordprof1 < 0 )
    {
	for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	    ppds_.setAngCoord( ippd, icoordprof0,
			       sectionangle_, sectionlength_ );
	return;
    }

    for ( int ippd=0; ippd<icoordprof0; ippd++ )
	ppds_.setInterpCoord( ippd, icoordprof0, icoordprof1 );

    while ( true )
    {
	for ( int ippd=icoordprof0+1; ippd<icoordprof1; ippd++ )
	    ppds_.setInterpCoord( ippd, icoordprof0, icoordprof1 );

	int inew = -1;
	for ( int ippd=icoordprof1+1; ippd<ppds_.size(); ippd++ )
	    if ( ppds_[ippd].hasCoord() )
		{ inew = ippd; break; }
	if ( inew < 0 )
	    break;

	icoordprof0 = icoordprof1;
	icoordprof1 = inew;
    }

    for ( int ippd=icoordprof1+1; ippd<ppds_.size(); ippd++ )
	ppds_.setInterpCoord( ippd, icoordprof0, icoordprof1 );
}


bool ProfileSetFromSingleEventCreator::canDoWork(
				const ProfileSet& pm )
{
    const int nruniqwells = pm.nrWells( true );
    if ( nruniqwells < 1 ) return false;
    if ( nruniqwells > 1 ) return true;

    // only if one single well is used multiple times we can do nothing useful
    const int nrwells = pm.nrWells( false );
    return nrwells == 1;
}


bool ProfileSetFromSingleEventCreator::doGo( TaskRunner* tr )
{
    if ( marker_.isEmpty() )
	mErrRet("Please specify a marker")

    getKnownDepths( *zvalprov_, marker_ );
    setMarkerDepths( marker_ );

    return true;
}



ProfileSetFromMultiEventCreator::ProfileSetFromMultiEventCreator(
	ProfileSet& m,
	const ObjectSet<ZValueProvider>& zprovs, const BufferStringSet& lvlnms,
	const TypeSet<Coord>& linegeom, int totalnrprofs )
    : ProfileSetFromEventCreator(m,totalnrprofs)
    , zvalprovs_(zprovs)
    , lvlnms_(lvlnms)
{
    const int nrwlls = profs_.nrWells();
    if ( maxnrprofs_ <= nrwlls )
	return;

    profs_.removeProfiles();
    ppds_.addExistingProfiles();
    addNewPositions();
    fillCoords( linegeom );
}


ProfileSetFromMultiEventCreator::~ProfileSetFromMultiEventCreator()
{
}


void ProfileSetFromMultiEventCreator::fillCoords(
					const TypeSet<Coord>& linegeom )
{
    const PolyLineND<Coord> pline( linegeom );
    const double lastarclen = pline.arcLength();
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& ppd = ppds_[ippd];
	if ( ppd.prof_ )
	    ppd.coord_ = pline.getPoint( ppd.prof_->pos_ * lastarclen );
    }
}


bool ProfileSetFromMultiEventCreator::doGo( TaskRunner* tr )
{
    for ( int idx=0; idx<zvalprovs_.size(); idx++ )
    {
	for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	{
	    ProfilePosData& ppd = ppds_[ippd];
	    ppd.zoffs_ = ppd.zhor_ = mUdf(float);
	}

	movepol_ = idx == 0 ? MoveAll : MoveBelow;
	const char* mrknm = lvlnms_.get(idx).buf();

	getKnownDepths( *zvalprovs_[idx], mrknm );
	setMarkerDepths( mrknm );
    }

    return true;
}
