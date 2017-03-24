/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "profilemodelcreator.h"
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

ProfilePosDataSet( ProfileModelBase& pms )
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

    ProfileModelBase& profs_;

};


ProfileModelFromEventData::Event::Event( ZValueProvider* zprov )
    : zvalprov_(zprov)
    , tiemarkernm_(ProfileModelFromEventData::addMarkerStr())
{
    BufferString defmrkrnm( "[", zprov->getName().getFullString(), "]" );
    newintersectmarker_ = new Well::Marker( defmrkrnm );
    newintersectmarker_->setColor( zprov->drawColor() );
    Strat::Level* newlevel =
	Strat::eLVLS().add( newintersectmarker_->name(),
			    newintersectmarker_->color() );
    levelid_ = newlevel->id();
}


ProfileModelFromEventData::Event::~Event()
{
    delete zvalprov_;
    delete newintersectmarker_;
}


BufferString ProfileModelFromEventData::Event::getMarkerName() const
{
    return newintersectmarker_ ? newintersectmarker_->name() : tiemarkernm_;
}


float ProfileModelFromEventData::getZValue( int evidx, const Coord& crd ) const
{
    if ( !events_.validIdx(evidx) )
	return mUdf(float);

    return events_[evidx]->zvalprov_->getZValue( crd );
}


BufferString ProfileModelFromEventData::getMarkerName( int evidx ) const
{
    if ( !events_.validIdx(evidx) )
	return BufferString::empty();

    return events_[evidx]->getMarkerName();
}


bool ProfileModelFromEventData::isIntersectMarker( int evidx ) const
{
    if ( !events_.validIdx(evidx) )
	return false;

    return events_[evidx]->newintersectmarker_;
}


void ProfileModelFromEventData::setTieMarker( int evidx, const char* markernm )
{
    if ( !events_.validIdx(evidx) )
	return;

    Event& ev = *events_[evidx];
    delete ev.newintersectmarker_;
    ev.newintersectmarker_ = 0;
    ev.tiemarkernm_ = markernm;
    Strat::eLVLS().remove( ev.levelid_ );

    FixedString markernmstr( markernm );
    if ( !markernm || markernmstr==ProfileModelFromEventData::addMarkerStr() )
    {
	ev.tiemarkernm_ = addMarkerStr();
	BufferString defmrkrnm(
		"[", ev.zvalprov_->getName().getFullString(), "]" );
	ev.newintersectmarker_ = new Well::Marker( defmrkrnm );
	ev.newintersectmarker_->setColor( ev.zvalprov_->drawColor() );
	Strat::Level* newlevel =
	    Strat::eLVLS().add( ev.newintersectmarker_->name(),
				ev.newintersectmarker_->color() );
	ev.levelid_ = newlevel->id();
    }
}


Well::Marker* ProfileModelFromEventData::getIntersectMarker( int evidx ) const
{
    if ( !events_.validIdx(evidx) )
	return 0;

    return events_[evidx]->newintersectmarker_;
}


void ProfileModelFromEventData::addEvent( ZValueProvider* zprov )
{
    events_ += new ProfileModelFromEventData::Event( zprov );
}


void ProfileModelFromEventData::removeEvent( int evidx )
{
    if ( !events_.validIdx(evidx) )
	return;

    delete events_.removeSingle( evidx );
}


ProfileModelFromEventData::ProfileModelFromEventData(
	ProfileModelBase& model, const TypeSet<Coord>& linegeom )
    : model_(model)
    , section_(linegeom)
{
}


ProfileModelFromEventData::~ProfileModelFromEventData()
{
    deepErase( events_ );
}


#define mErrRet(s) { errmsg_ = s; return false; }


ProfileModelFromEventCreator::ProfileModelFromEventCreator( ProfileModelBase& p,
							    int mxp )
    : profs_(p)
    , ppds_(*new ProfilePosDataSet(p))
    , t2dtr_(0)
    , t2dpar_(*new IOPar)
    , totalnrprofs_(mxp>0 ? mxp : 50)
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


ProfileModelFromEventCreator::~ProfileModelFromEventCreator()
{
    if ( t2dtr_ )
	{ t2dtr_->unRef(); t2dtr_ = 0; }
    t2dpar_.setEmpty();
    delete &ppds_;
    delete &t2dpar_;
}


void ProfileModelFromEventCreator::preparePositions()
{
    ppds_.addExistingProfiles();
    addNewPositions();
    fillCoords();
}

void ProfileModelFromEventCreator::prepareZTransform( TaskRunner* tr )
{
    if ( !needt2d_ )
    {
	if ( t2dtr_ )
	    t2dtr_->unRef();
	t2dtr_ = 0;
	return;
    }


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
}


float ProfileModelFromEventCreator::getDepthVal( const Coord& pos,
						 float zval ) const
{
    if ( !needt2d_ || mIsUdf(zval) )
	return zval;

    if ( !t2dtr_ )
	{ pErrMsg("No T2D"); zval *= 2000; }
    else
	zval = t2dtr_->transform( Coord3(pos,zval) );
    return zval;
}


bool ProfileModelFromSingleEventCreator::setIntersectMarkers()
{
    if ( !event_.newintersectmarker_ )
	return true;

    for ( int iprof=0; iprof<profs_.size(); iprof++ )
    {
	ProfileBase* prof = profs_.get( iprof );
	if ( prof->isWell() )
	{
	    Well::Marker* newtiemarker = new Well::Marker(
		    *event_.newintersectmarker_ );
	    float dah = event_.zvalprov_->getZValue( prof->coord_ );
	    if ( mIsUdf(dah) )
		continue;

	    dah = getDepthVal( prof->coord_, dah );
	    newtiemarker->setDah( dah );
	    prof->markers_.insertNew( newtiemarker );
	}
    }

    if ( !interpolateIntersectMarkersUsingEV(event_) )
	return false;

    sortMarkers();
    return true;
}


float ProfileModelFromEventCreator::getInterpolatedDepth(
	int ippd, const ProfileModelFromEventData::Event& event ) const
{
    const ZValueProvider* zvalprov = event.zvalprov_;

    int prevppdidx = ippd;
    float prevppdmdepth = mUdf(float);
    float prevpos = mUdf(float);
    while ( --prevppdidx && prevppdidx>=0 )
    {
	const ProfilePosData& curppd = ppds_[prevppdidx];
	prevppdmdepth = zvalprov->getZValue( curppd.coord_ );
	prevppdmdepth = getDepthVal( curppd.coord_, prevppdmdepth );
	prevpos = curppd.pos_;
	if ( !mIsUdf(prevppdmdepth) )
	    break;
    }

    int nextppdidx = ippd;
    float nextppdmdepth = mUdf(float);
    float nextpos = mUdf(float);
    while ( ++nextppdidx && nextppdidx<ppds_.size() )
    {
	const ProfilePosData& curppd = ppds_[nextppdidx];
	nextppdmdepth = zvalprov->getZValue( curppd.coord_ );
	nextppdmdepth = getDepthVal( curppd.coord_, nextppdmdepth );
	nextpos = curppd.pos_;
	if ( !mIsUdf(nextppdmdepth) )
	    break;
    }

    if ( mIsUdf(prevppdmdepth) || mIsUdf(nextppdmdepth) )
	return mUdf(float);

    const float curpos = ppds_[ippd].pos_;
    const float relpos = (curpos - prevpos) / (nextpos - prevpos);
    return (prevppdmdepth*(1-relpos)) + (nextppdmdepth*relpos);
}


bool ProfileModelFromEventCreator::interpolateIntersectMarkersUsingEV(
	const ProfileModelFromEventData::Event& ev )
{
    BufferString markernm = ev.getMarkerName();
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	const ProfilePosData ppd = ppds_[ippd];
	if ( !ppd.isWell() )
	    continue;

	ProfileBase* prof = ppd.prof_;
	Well::Marker* marker = prof->markers_.getByName( markernm );
	if ( !marker )
	{
	    pErrMsg( "Cannot find well marker intersections" );
	    return false;
	}

	if ( mIsUdf(marker->dah()) )
	{
	    const float idah = getInterpolatedDepth( ippd, ev );
	    if ( mIsUdf(idah) )
	    {
		pErrMsg("Cant extrapolate wellmarker intersection");
		return false;
	    }

	    marker->setDah( idah );
	}
    }

    return true;
}


void ProfileModelFromEventCreator::sortMarkers()
{
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	if ( !ppds_[ippd].prof_ )
	    continue;

	Well::MarkerSet& profmarkers = ppds_[ippd].prof_->markers_;
	profmarkers.sortByDAH();
    }
}



bool ProfileModelFromEventCreator::go( TaskRunner* tr )
{
    errmsg_.setEmpty();
    preparePositions();
    prepareZTransform( tr );
    setIntersectMarkers();
    setNewProfiles();

    if ( doGo( tr ) )
    {
	profs_.removeAtSamePos();
	return true;
    }

    return false;
}


void ProfileModelFromEventCreator::addNewPositions()
{
    const int oldsz = ppds_.size();
    const int nrnew = totalnrprofs_ - oldsz;
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
	ippd = addNewPPDsAfter( ippd, nr2add[idx], idx == oldsz );
}


int ProfileModelFromEventCreator::addNewPPDsAfter( int ippd,
					int nr2add, bool islast )
{
    if ( nr2add < 1 ) return ippd + 1;
    const int nrppds = ppds_.size();

    const int ippdbefore = ippd > -1 ? ippd : 0;
    const int ippdafter = ippd < nrppds-1 ? ippd+1 : nrppds - 1;
    const ProfilePosData& ppd0 = ppds_[ippdbefore];
    const ProfilePosData& ppd1 = ppds_[ippdafter];
    const float pos0 = ippd > -1 ? ppd0.pos_ : 0.f;
    const float pos1 = islast ? 1.f : ppd1.pos_;

    const float dpos = (pos1 - pos0) / (nr2add+1);
    int inewppd = ippd + 1;
    for ( int iadd=0; iadd<nr2add; iadd++ )
    {
	const float newpos = pos0 + (iadd+1) * dpos;
	ppds_.insert( inewppd, ProfilePosData(0,newpos) );
	inewppd++;
    }

    return inewppd;
}


void ProfileModelFromEventCreator::setNewProfiles()
{
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& curppd = ppds_[ippd];
	if ( curppd.prof_ )
	    continue;

	int prevppdidx = ippd;
	while ( --prevppdidx && prevppdidx>=0 )
	{
	    const ProfilePosData& prevppd = ppds_[prevppdidx];
	    if ( prevppd.prof_ )
		break;
	}
	const ProfileBase* prevprof =
	    ppds_.validIdx(prevppdidx) ? ppds_[prevppdidx].prof_ : 0;

	int nextppdidx = ippd;
	while ( ++nextppdidx && nextppdidx<ppds_.size() )
	{
	    const ProfilePosData& nextppd = ppds_[nextppdidx];
	    if ( nextppd.prof_ )
		break;
	}
	const ProfileBase* nextprof =
	    ppds_.validIdx(nextppdidx) ? ppds_[nextppdidx].prof_ : 0;

	ProfileBase* newprof = ProfFac().create( curppd.pos_ );
	newprof->createMarkersFrom( prevprof, nextprof );
	curppd.prof_ = newprof;
	profs_.set( newprof, false );
    }
}


void ProfileModelFromEventCreator::getKnownDepths(
	const ProfileModelFromEventData::Event& ev )
{
    getEventZVals( ev );
    getZOffsets( ev );
}


void ProfileModelFromEventCreator::getEventZVals(
	const ProfileModelFromEventData::Event& ev )
{
    // Get the depths from the horizon + transform
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& ppd = ppds_[ippd];
	ppd.zhor_ = ev.zvalprov_->getZValue( ppd.coord_ );
	if ( !ppd.hasHorZ() )
	    continue;

	ppd.zhor_ = getDepthVal( ppd.coord_, ppd.zhor_ );
    }

    Array1DImpl<float> zvals( ppds_.size() );
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	zvals.set( ippd, ppds_[ippd].zhor_ );
    LinearArray1DInterpol zvalinterp;
    zvalinterp.setExtrapol( true );
    zvalinterp.setFillWithExtremes( true );
    zvalinterp.setArray( zvals );
    zvalinterp.execute();
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ppds_[ippd].zhor_ = zvals.get( ippd );
	ProfileBase* prof = ppds_[ippd].prof_;
	const int markeridx = prof->markers_.indexOf( ev.getMarkerName() );
	if ( markeridx>=0 && ev.newintersectmarker_ &&
	     mIsUdf(prof->markers_[markeridx]->dah()) )
	     prof->markers_[markeridx]->setDah( zvals.get(ippd) );
    }
}


void ProfileModelFromEventCreator::getZOffsets(
	const ProfileModelFromEventData::Event& ev )
{
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& ppd = ppds_[ippd];
	if ( ppd.isWell() )
	{
	    const Well::Marker* mrkr =
		ppd.prof_->markers_.getByName( ev.getMarkerName() );
	    ppd.zoffs_ =
		mrkr && !mIsUdf(mrkr->dah()) ? mrkr->dah() - ppd.zhor_
					     : mUdf(float);
	}
    }
}


void ProfileModelFromEventCreator::interpolateZOffsets()
{
    bool haszoffs = false;
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	if ( ppds_[ippd].hasZOffset() )
	    { haszoffs = true; break; }
    }

    if ( !haszoffs )
    {
	for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	    ppds_[ippd].zoffs_ = 0;
	return;
    }

    Array1DImpl<float> offszvals( ppds_.size() );
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	offszvals.set( ippd, ppds_[ippd].zoffs_ );
    LinearArray1DInterpol offszvalinterp;
    offszvalinterp.setExtrapol( true );
    offszvalinterp.setFillWithExtremes( true );
    offszvalinterp.setArray( offszvals );
    offszvalinterp.execute();
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	ppds_[ippd].zoffs_ = offszvals.get( ippd );
}


void ProfileModelFromEventCreator::setMarkerDepths(
	const ProfileModelFromEventData::Event& ev )
{
    interpolateZOffsets();

    if ( ppds_.isEmpty() || !ppds_[0].hasHorZ() )
	return;

    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& ppd = ppds_[ippd];
	if ( !ppd.prof_ || ppd.isWell() )
	    continue;

	const int midx = ppd.prof_->markers_.indexOf( ev.getMarkerName() );
	if ( midx >= 0 )
	{
	    const float orgz = ppd.prof_->markers_[midx]->dah();
	    const float newz = ppd.zhor_ + ppd.zoffs_;
	    ppd.prof_->moveMarker( midx, newz,
				   doPush(orgz,newz), doPull(orgz,newz) );
	}
    }
}


bool ProfileModelFromEventCreator::doPush( float orgz, float newz ) const
{
    if ( movepol_ < MoveAbove )
	return movepol_ == MoveAll;
    return (orgz < newz && movepol_ == MoveBelow)
	|| (orgz > newz && movepol_ == MoveAbove);
}


bool ProfileModelFromEventCreator::doPull( float orgz, float newz ) const
{
    if ( movepol_ < MoveAbove )
	return movepol_ == MoveAll;
    return (orgz > newz && movepol_ == MoveBelow)
	|| (orgz < newz && movepol_ == MoveAbove);
}



ProfileModelFromSingleEventCreator::ProfileModelFromSingleEventCreator(
	ProfileModelBase& m, const ProfileModelFromEventData::Event& ev )
    : ProfileModelFromEventCreator(m)
    , event_(ev)
    , sectionangle_(0)
    , sectionlength_(1000)
{
}


void ProfileModelFromSingleEventCreator::fillCoords()
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


bool ProfileModelFromSingleEventCreator::canDoWork(
				const ProfileModelBase& pm )
{
    const int nruniqwells = pm.nrWells( true );
    if ( nruniqwells < 1 ) return false;
    if ( nruniqwells > 1 ) return true;

    // only if one single well is used multiple times we can do nothing useful
    const int nrwells = pm.nrWells( false );
    return nrwells == 1;
}


bool ProfileModelFromSingleEventCreator::doGo( TaskRunner* tr )
{
    if ( event_.tiemarkernm_.isEmpty() )
	mErrRet("Please specify a marker")

    getKnownDepths( event_ );
    setMarkerDepths( event_ );

    return true;
}



ProfileModelFromMultiEventCreator::ProfileModelFromMultiEventCreator(
	ProfileModelFromEventData& data )
    : ProfileModelFromEventCreator(data.model_,data.totalnrprofs_)
    , data_(data)
{
    const int nrwlls = profs_.nrWells();
    if ( totalnrprofs_ <= nrwlls )
	return;

    profs_.removeProfiles();
}


ProfileModelFromMultiEventCreator::~ProfileModelFromMultiEventCreator()
{
}


float ProfileModelFromMultiEventCreator::getDepthValue(
	int evidx, const ProfileBase& prof )
{
    float evdepthval = data_.getZValue( evidx, prof.coord_ );
    if ( mIsUdf(evdepthval) )
	return mUdf(float);

    return getDepthVal( prof.coord_, evdepthval );
}


float ProfileModelFromMultiEventCreator::getZOffset(
	int evidx, const ProfileBase& prof )
{
    const float evdepthval = getDepthValue( evidx, prof );
    if ( mIsUdf(evdepthval) )
	return mUdf(float);

    const Well::Marker* mrkr =
	prof.markers_.getByName( data_.getMarkerName(evidx) );
    return mrkr && !mIsUdf(mrkr->dah()) ? mrkr->dah() - evdepthval
					: mUdf(float);
}


float ProfileModelFromMultiEventCreator::getIntersectMarkerDepth(
	int evidx, const ProfileBase& prof )
{
    float topzoffs = mUdf(float);
    float topz = mUdf(float);
    const int topevidx = evidx -1;
    if ( topevidx >= 0 )
    {
	topzoffs = getZOffset( topevidx, prof );
	topz = getDepthValue( topevidx, prof );
    }

    int bottomevidx = evidx;
    float bottomzoffs = mUdf(float);
    float bottomz = mUdf(float);
    while( ++bottomevidx && bottomevidx<data_.nrEvents() )
    {
	if ( !data_.isIntersectMarker(bottomevidx) )
	{
	    bottomzoffs = getZOffset( bottomevidx, prof );
	    bottomz = getDepthValue( bottomevidx, prof );
	    break;
	}
    }

    const float actevz = getDepthValue( evidx, prof );
    if ( mIsUdf(topzoffs) && mIsUdf(bottomzoffs) )
	return actevz;

    const float evrelpos = (actevz - topz) / (bottomz-topz);
    float evzoffs = mUdf(float);
    if ( mIsUdf(topzoffs) )
	evzoffs = bottomzoffs;
    else if ( mIsUdf(bottomzoffs) )
	evzoffs = topzoffs;
    else
	evzoffs = (topzoffs * (1-evrelpos)) + (bottomzoffs * evrelpos);

    return actevz + evzoffs;
}


bool ProfileModelFromMultiEventCreator::setIntersectMarkers()
{
    for ( int evidx=0; evidx<data_.nrEvents(); evidx++ )
    {
	if ( data_.isIntersectMarker(evidx) )
	{
	    for ( int iprof=0; iprof<profs_.size(); iprof++ )
	    {
		ProfileBase* prof = profs_.get( iprof );
		if ( prof->isWell() )
		{
		    Well::Marker* newtiemarker = new Well::Marker(
			    *data_.getIntersectMarker(evidx) );
		    newtiemarker->setDah(getIntersectMarkerDepth(evidx,*prof));
		    prof->markers_.insertNew( newtiemarker );
		}
	    }
	}
    }

    if ( !interpolateIntersectMarkers() )
	return false;

    sortMarkers();
    return true;
}


bool ProfileModelFromMultiEventCreator::interpolateIntersectMarkers()
{
    for ( int evidx=0; evidx<data_.nrEvents(); evidx++ )
    {
	const ProfileModelFromEventData::Event& ev = *data_.events_[evidx];
	if ( !ev.newintersectmarker_ )
	    continue;

	if ( !interpolateIntersectMarkersUsingEV(ev) )
	    return false;
    }

    return true;
}


void ProfileModelFromMultiEventCreator::fillCoords()
{
    const PolyLineND<Coord> pline( data_.section_.linegeom_ );
    const double lastarclen = pline.arcLength();
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& ppd = ppds_[ippd];
	ppd.coord_ = pline.getPoint( ppd.pos_ * lastarclen );
    }
}


bool ProfileModelFromMultiEventCreator::doGo( TaskRunner* tr )
{
    for ( int evidx=0; evidx<data_.nrEvents(); evidx++ )
    {
	for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	{
	    ProfilePosData& ppd = ppds_[ippd];
	    ppd.zoffs_ = ppd.zhor_ = mUdf(float);
	}

	movepol_ = evidx == 0 ? MoveAll : MoveBelow;
	getKnownDepths( *data_.events_[evidx] );
	setMarkerDepths( *data_.events_[evidx] );
    }

    return true;
}
