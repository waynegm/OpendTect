/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "profilemodelcreator.h"

#include "ioman.h"
#include "ioobj.h"
#include "profilebase.h"
#include "profileposprovider.h"
#include "array1dinterpol.h"
#include "arrayndimpl.h"
#include "sorting.h"
#include "survinfo.h"
#include "uistrings.h"
#include "zaxistransform.h"
#include "zvalueprovider.h"
#include <math.h>
#include <iostream>


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
    : model_(pms)
{
}

void addExistingProfiles()
{
    for ( int iprof=0; iprof<model_.size(); iprof++ )
    {
	ProfileBase* prof = model_.get( iprof );
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

    ProfileModelBase& model_;
};


#define mErrRet(s) { errmsg_ = s; return false; }


ProfileModelFromEventCreator::ProfileModelFromEventCreator(
	ProfileModelBase& p, ProfilePosProvider* posprov, int mxp )
    : model_(p)
    , profposprov_(posprov)
    , ppds_(*new ProfilePosDataSet(p))
    , t2dtr_(0)
    , t2dpar_(*new IOPar)
    , totalnrprofs_(mxp>0 ? mxp : 50)
    , movepol_(MoveAll)
    , needt2d_(SI().zIsTime()) // just def
{
    for ( int idx=0; idx<model_.size(); idx++ )
    {
	ProfileBase* prof = model_.get( idx );
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
    delete profposprov_;
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
    sortMarkers();
    setNewProfiles();

    if ( doGo( tr ) )
    {
	model_.removeAtSamePos();
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
	model_.set( newprof, false );
    }
}


void ProfileModelFromEventCreator::getKnownDepths(
	const ProfileModelFromEventData::Event& ev )
{
    setEventZVals( ev );
    setZOffsets( ev );
}


void ProfileModelFromEventCreator::setEventZVals(
	const ProfileModelFromEventData::Event& ev )
{
    // Get the depths from the horizon + transform
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& ppd = ppds_[ippd];
	ppd.zhor_ = ev.zvalprov_->getZValue( ppd.coord_ );
	if ( !ppd.hasHorZ() || !SI().zIsTime() )
	    continue;

	ppd.zhor_ = model_.getDepthVal( ppd.zhor_, ppd.pos_ );
    }

    Array1DImpl<float> zvals( ppds_.size() );
    bool hasundef = false;
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	const float zhor = ppds_[ippd].zhor_;
	if ( mIsUdf(zhor) )
	    hasundef = true;
	zvals.set( ippd, zhor );
    }


    if ( hasundef )
    {
	LinearArray1DInterpol zvalinterp;
	zvalinterp.setExtrapol( true );
	zvalinterp.setFillWithExtremes( true );
	zvalinterp.setArray( zvals );
	zvalinterp.execute();
	for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	    ppds_[ippd].zhor_ = zvals.get( ippd );
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

	    if ( isSingleEvent() )
		ppd.prof_->moveMarker( midx, newz,
				       doPush(orgz,newz), doPull(orgz,newz) );
	    else
		ppd.prof_->markers_[midx]->setDah( newz );
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


void ProfileModelFromEventCreator::fillCoords()
{
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
	ppds_[ippd].coord_ = profposprov_->getCoord( ppds_[ippd].pos_ );
}


ProfileModelFromSingleEventCreator::ProfileModelFromSingleEventCreator(
	ProfileModelBase& m, const ProfileModelFromEventData::Event& ev,
	ProfilePosProvider* posprov )
    : ProfileModelFromEventCreator(m,posprov)
    , event_(ev)
{
}


bool ProfileModelFromSingleEventCreator::doGo( TaskRunner* taskrunner )
{
    if ( event_.tiemarkernm_.isEmpty() )
	mErrRet( tr("Please specify a marker") )

    getKnownDepths( event_ );
    setMarkerDepths( event_ );

    return true;
}


void ProfileModelFromSingleEventCreator::setZOffsets(
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



ProfileModelFromMultiEventCreator::ProfileModelFromMultiEventCreator(
	ProfileModelFromEventData& data, ProfilePosProvider* posprov )
    : ProfileModelFromEventCreator(*data.model_,posprov,data.totalnrprofs_)
    , data_(data)
{
    const int nrwlls = model_.nrWells();
    if ( totalnrprofs_ <= nrwlls )
	return;

    model_.removeProfiles();
}


ProfileModelFromMultiEventCreator::~ProfileModelFromMultiEventCreator()
{
}


void ProfileModelFromMultiEventCreator::setZOffsets(
	const ProfileModelFromEventData::Event& ev )
{
    for ( int ippd=0; ippd<ppds_.size(); ippd++ )
    {
	ProfilePosData& ppd = ppds_[ippd];
	const Well::Marker* mrkr =
	    ppd.prof_->markers_.getByName( ev.getMarkerName() );
	if ( ppd.isWell() )
	    ppd.zoffs_ = mrkr && !mIsUdf(mrkr->dah()) ? mrkr->dah() - ppd.zhor_
						      : mUdf(float);
    }
}



void ProfileModelFromMultiEventCreator::reArrangeMarkers()
{
    for ( int iev=0; iev<data_.events_.size()-1; iev++ )
    {
	const BufferString evmarkernm = data_.events_[iev]->getMarkerName();
	const BufferString lowevmarkernm =
	    data_.events_[iev+1]->getMarkerName();
	for ( int iprof=0; iprof<model_.size(); iprof++ )
	{
	    ProfileBase* prof = model_.get( iprof );
	    if ( prof->isWell() )
		continue;

	    const int evmarkeridx = prof->markers_.indexOf( evmarkernm );
	    const int lowevmarkeridx = prof->markers_.indexOf( evmarkernm );
	    if ( evmarkeridx<0 || lowevmarkeridx<0 )
		continue;

	    const float evmarkerdah = prof->markers_[evmarkeridx]->dah();
	    const float lowevmarkerdah = prof->markers_[lowevmarkeridx]->dah();
	    if ( evmarkerdah>lowevmarkerdah )
		prof->markers_[lowevmarkeridx]->setDah( evmarkerdah );

	}
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

    interpolateMarkersBetweenEvents();
    reArrangeMarkers();
    return true;
}


int ProfileModelFromMultiEventCreator::getTopBottomEventMarkerIdx(
	const ProfileBase& prof, int imarker, bool findtop ) const
{
    const int incrmidx = findtop ? -1 : +1;
    int resmidx = imarker;
    while ( true )
    {
	resmidx+=incrmidx;
	if ( resmidx<0 || resmidx>=prof.markers_.size() )
	    return -1;
	const char* markernm = prof.markers_[resmidx]->name().buf();
	const int tieevidx = data_.tiedToEventIdx( markernm );
	if ( tieevidx>=0 )
	    break;
    }

    return resmidx;
}

bool ProfileModelFromMultiEventCreator::getTopBotEventValsWRTMarker(
	const char* markernm, const ProfileBase& prof, float& mval,
	float& topevmval, float& botevmval, float& relmpos ) const
{
    const int midx = prof.markers_.indexOf( markernm );
    if ( midx<0 )
	return false;

    const int topmidx = getTopBottomEventMarkerIdx( prof, midx, true );
    const int botmidx = getTopBottomEventMarkerIdx( prof, midx, false );

    const Well::MarkerSet& mrkrs = prof.markers_;
    mval = mrkrs[midx]->dah();
    if ( mrkrs.validIdx(topmidx) )
	topevmval = mrkrs[topmidx]->dah();
    if ( mrkrs.validIdx(botmidx) )
	botevmval = mrkrs[botmidx]->dah();
    if ( !mIsUdf(topevmval) && !mIsUdf(botevmval) )
	relmpos = (mval-topevmval) / (botevmval-topevmval);
    return true;
}

#define mPrrContinue \
{ \
    pErrMsg( "Cant interpolate" ); \
    continue; \
}


void ProfileModelFromMultiEventCreator::interpolateMarkersBetweenEvents()
{
    for ( int iprof=0; iprof<model_.size(); iprof++ )
    {
	ProfileBase* prof = model_.get( iprof );
	if ( prof->isWell() )
	    continue;
	Well::MarkerSet& markers = prof->markers_;
	const int leftwellidx = model_.indexBefore( prof->pos_, true );
	const int rightwellidx = model_.indexAfter( prof->pos_, true );
	const ProfileBase* leftprof =
	    leftwellidx<0 ? 0 : model_.get( leftwellidx );
	const ProfileBase* rightprof =
	    rightwellidx<0 ? 0 : model_.get( rightwellidx );
	if ( !leftprof && !rightprof )
	{
	    pErrMsg( "Huh.. No left & right well!" );
	    continue;
	}

	const float disttoleftfac =
	    leftprof && rightprof ? 1 - (prof->pos_ - leftprof->pos_)/
					(rightprof->pos_-leftprof->pos_) : 0;
	const float disttorightfac =
	    rightprof && leftprof ? 1 - (rightprof->pos_ - prof->pos_)/
					(rightprof->pos_-leftprof->pos_) : 0;
	for ( int im=0; im<markers.size(); im++ )
	{
	    const char* markernm = markers[im]->name().buf();
	    const int tiedevidx = data_.tiedToEventIdx( markernm );
	    if ( tiedevidx>= 0 )
		continue;

	    const int topmidx = getTopBottomEventMarkerIdx( *prof, im, true );
	    const int botmidx = getTopBottomEventMarkerIdx( *prof, im, false );
	    const float topevdah =
		markers.validIdx(topmidx) ? markers[topmidx]->dah()
					  : mUdf(float);
	    const float botevdah =
		markers.validIdx(botmidx) ? markers[botmidx]->dah()
					  : mUdf(float);
	    if ( mIsUdf(topevdah) && mIsUdf(botevdah) )
	    {
		pErrMsg( "Huh! cannot find top & bottom event to interpolate.");
		continue;
	    }

	    float leftevdah, lefttopevdah, leftbotevdah, leftmrelpos;
	    leftevdah =  lefttopevdah = leftbotevdah = leftmrelpos =mUdf(float);
	    if ( leftprof )
		getTopBotEventValsWRTMarker( markernm, *leftprof, leftevdah,
					     lefttopevdah, leftbotevdah,
					     leftmrelpos );

	    float rightevdah, righttopevdah, rightbotevdah, rightmrelpos;
	    rightevdah =righttopevdah=rightbotevdah=rightmrelpos = mUdf(float);
	    if ( rightprof )
		getTopBotEventValsWRTMarker( markernm, *rightprof, rightevdah,
					     righttopevdah, rightbotevdah,
					     rightmrelpos );
	    float resultdah = mUdf(float);
	    float ddahval = mUdf(float);
	    if ( mIsUdf(topevdah) )
	    {
		if ( !leftprof )
		{
		    if ( mIsUdf(rightbotevdah) || mIsUdf(rightevdah) )
			mPrrContinue;
		    ddahval = rightevdah - rightbotevdah;
		}
		else if ( !rightprof )
		{
		    if ( mIsUdf(leftbotevdah) || mIsUdf(leftevdah) )
			mPrrContinue;
		    ddahval = leftevdah - leftbotevdah;
		}
		else
		    ddahval = disttoleftfac*(leftevdah - leftbotevdah) +
			      disttorightfac*(rightevdah - rightbotevdah);

		resultdah = botevdah + ddahval;
	    }
	    else if ( mIsUdf(botevdah) )
	    {
		if ( !leftprof )
		{
		    if ( mIsUdf(righttopevdah) || mIsUdf(rightevdah) )
			mPrrContinue;
		    ddahval = rightevdah - righttopevdah;
		}
		else if ( !rightprof )
		{
		    if ( mIsUdf(leftbotevdah) || mIsUdf(leftevdah) )
			mPrrContinue;
		    ddahval = leftevdah - lefttopevdah;
		}
		else
		    ddahval = disttoleftfac*(leftevdah - lefttopevdah) +
			      disttorightfac*(rightevdah - righttopevdah);

		resultdah = topevdah + ddahval;
	    }
	    else
	    {
		float resrelpos = mUdf(float);
		if ( mIsUdf(leftmrelpos) )
		    resrelpos = rightmrelpos;
		else if ( mIsUdf(rightmrelpos) )
		    resrelpos = leftmrelpos;
		else
		    resrelpos = ((disttoleftfac*leftmrelpos)+
				 (disttorightfac*rightmrelpos));
		resultdah = topevdah + resrelpos*(botevdah-topevdah);
	    }

	    markers[im]->setDah( resultdah );
	}
    }
}
