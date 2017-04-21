/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : April 2017
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "profilemodelfromeventdata.h"

#include "profilebase.h"
#include "profileposprovider.h"
#include "polylinend.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "uistrings.h"
#include "zvalueprovider.h"
#include <math.h>


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
	const int firstwellidx = model_.indexAfter( -1.0f, true );
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

    int prevposidx = model_.indexBefore( pos, true );
    int afterposidx = model_.indexAfter( pos, true );
    if ( prevposidx==-1 && afterposidx==-1 )
    {
	pErrMsg( "No positions found to get coordinate" );
	return Coord::udf();
    }

    if ( prevposidx<0 )
    {
	const float afterpos = model_.get( afterposidx )->pos_;
	prevposidx = afterposidx;
	afterposidx = model_.indexAfter( afterpos, true );
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
	prevposidx = model_.indexBefore( prevpos, true );
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


ProfileModelFromEventData::Event::Event( ZValueProvider* zprov )
    : zvalprov_(zprov)
    , newintersectmarker_(0)
{
    setMarker( zprov->getName().getFullString() );
}

void ProfileModelFromEventData::Event::setMarker( const char* markernm )
{
    Strat::LevelSet& lvls = Strat::eLVLS();
    if ( newintersectmarker_ )
	lvls.remove( levelid_ );
    delete newintersectmarker_;
    newintersectmarker_ = 0;
    FixedString markernmstr( markernm );
    const bool invalidmarkernm =
	markernmstr.isEmpty() ||
	markernmstr==ProfileModelFromEventData::addMarkerStr();
    if ( !lvls.isPresent(markernm) || invalidmarkernm )
    {
	newintersectmarker_ = new Well::Marker(
		invalidmarkernm ? zvalprov_->getName().getFullString()
				: markernmstr );
	newintersectmarker_->setColor( zvalprov_->drawColor() );
	Strat::Level* newlevel =
	    lvls.add( newintersectmarker_->name(),newintersectmarker_->color());
	tiemarkernm_ = markernm;
	levelid_ = newlevel->id();
	newintersectmarker_->setLevelID( levelid_ );
    }
    else
    {
	const int lvlidx = lvls.indexOf( markernm );
	tiemarkernm_ = markernmstr;
	levelid_ = lvls.levelID( lvlidx );
    }
}


ProfileModelFromEventData::Event::~Event()
{
    delete zvalprov_;
    if ( newintersectmarker_ )
	Strat::eLVLS().remove( levelid_ );
    delete newintersectmarker_;
}


BufferString ProfileModelFromEventData::Event::getMarkerName() const
{
    return newintersectmarker_ ? newintersectmarker_->name() : tiemarkernm_;
}


ProfileModelFromEventData::ProfileModelFromEventData(
	ProfileModelBase& model, const TypeSet<Coord>& linegeom )
    : model_(model)
    , section_(linegeom)
{
}


ProfileModelFromEventData::~ProfileModelFromEventData()
{
    removeAllEvents();
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


bool ProfileModelFromEventData::isIntersectMarker( const char* markernm ) const
{
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	if ( events_[iev]->newintersectmarker_ &&
	     events_[iev]->newintersectmarker_->name()==markernm )
	    return true;
    }

    return false;
}


bool ProfileModelFromEventData::isIntersectMarker( int evidx ) const
{
    if ( !events_.validIdx(evidx) )
	return false;

    return events_[evidx]->newintersectmarker_;
}


int ProfileModelFromEventData::tiedToEventIdx( const char* markernm ) const
{
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	if ( events_[iev]->getMarkerName()==markernm )
	    return iev;
    }

    return -1;
}


void ProfileModelFromEventData::setTieMarker( int evidx, const char* markernm )
{
    if ( !events_.validIdx(evidx) )
	return;

    Event& ev = *events_[evidx];
    if ( ev.newintersectmarker_ )
	model_.removeMarker( ev.newintersectmarker_->name() );

    ev.setMarker( markernm );
}


#define maxAllowedDDah 50

void ProfileModelFromEventData::setNearestTieEvent(
	int ev1idx, int ev2idx, const BufferString& tiemnm )
{
    const int tiedtoevidx = tiedToEventIdx( tiemnm );
    const float ev1avgdzval = getAvgDZval( ev1idx, tiemnm );
    const float ev2avgdzval = getAvgDZval( ev2idx, tiemnm );
    if ( ev1avgdzval<ev2avgdzval )
    {
	if ( tiedtoevidx==ev1avgdzval )
	    setTieMarker( ev2idx, addMarkerStr() );
	else
	{
	    setTieMarker( ev2idx, addMarkerStr() );
	    setTieMarker( ev1idx, tiemnm );
	}
    }
    else if ( ev2avgdzval<ev1avgdzval )
    {
	if ( tiedtoevidx==ev2avgdzval )
	    setTieMarker( ev1idx, addMarkerStr() );
	else
	{
	    setTieMarker( ev1idx, addMarkerStr() );
	    setTieMarker( ev2idx, tiemnm );
	}
    }
}


float ProfileModelFromEventData::getAvgDZval(
	int evidx, const BufferString& markernm ) const
{
    Stats::CalcSetup su;
    su.require( Stats::Average );
    Stats::RunCalc<float> statrc( su );
    for ( int iprof=0; iprof<model_.size(); iprof++ )
    {
	const ProfileBase* prof = model_.get( iprof );
	if ( !prof->isWell() )
	    continue;

	const float evdah = getEventDepthVal( evidx, *prof );
	const int markeridx = prof->markers_.indexOf( markernm );
	if ( markeridx<0 )
	    continue;

	statrc.addValue( prof->markers_[markeridx]->dah()-evdah );
    }

    return fabs( statrc.average() );
}


bool ProfileModelFromEventData::findTieMarker( int evidx,
					       BufferString& markernm ) const
{
    if ( !isIntersectMarker(evidx) )
	return true;

    BufferStringSet nearestmarkernms;
    for ( int iprof=0; iprof<model_.size(); iprof++ )
    {
	const ProfileBase* prof = model_.get( iprof );
	if ( !prof->isWell() )
	    continue;

	const float evdah = getEventDepthVal( evidx, *prof );
	if ( mIsUdf(evdah) )
	    continue;

	const int topmarkeridx = prof->markers_.getIdxAbove( evdah );
	const int botmarkeridx = prof->markers_.getIdxBelow( evdah );
	if ( !prof->markers_.validIdx(topmarkeridx) &&
	     !prof->markers_.validIdx(botmarkeridx) )
	{
	    pErrMsg( "No top & bottom marker" );
	    continue;
	}

	if ( !prof->markers_.validIdx(topmarkeridx) )
	{
	    nearestmarkernms.addIfNew( prof->markers_[botmarkeridx]->name() );
	    continue;
	}
	else if ( !prof->markers_.validIdx(botmarkeridx) )
	{
	    nearestmarkernms.addIfNew( prof->markers_[topmarkeridx]->name() );
	    continue;
	}

	const float topmarkerddah =
	    evdah - prof->markers_[topmarkeridx]->dah();
	const float botmarkerddah =
	    prof->markers_[botmarkeridx]->dah() - evdah;
	nearestmarkernms.addIfNew(
	     topmarkerddah<botmarkerddah ? prof->markers_[topmarkeridx]->name()
					 :prof->markers_[botmarkeridx]->name());
    }

    if ( nearestmarkernms.size()==1 )
    {
	markernm = nearestmarkernms.get( 0 );
	return true;
    }

    TypeSet<float> nearestavgddahs;
    nearestavgddahs.setSize( nearestmarkernms.size(), 0.f );
    for ( int idx=0; idx<nearestmarkernms.size(); idx++ )
    {
	Stats::CalcSetup su;
	su.require( Stats::Average );
	Stats::RunCalc<float> statrc( su );
	for ( int iprof=0; iprof<model_.size(); iprof++ )
	{
	    const ProfileBase* prof = model_.get( iprof );
	    if ( !prof->isWell() )
		continue;

	    const float evdah = getEventDepthVal( evidx, *prof );
	    if ( mIsUdf(evdah) )
		continue;

	    const int nearestmarkeridx =
		prof->markers_.indexOf( nearestmarkernms.get(idx) );
	    if ( nearestmarkeridx<0 )
		continue;

	    statrc.addValue( prof->markers_[nearestmarkeridx]->dah()-evdah );
	}

	nearestavgddahs[idx] = fabs(statrc.average());
    }

    int nearestmidx = -1;
    float nearestddah = mUdf(float);
    for ( int idx=0; idx<nearestavgddahs.size(); idx++ )
    {
	if ( nearestavgddahs[idx]<nearestddah )
	{
	    nearestddah = nearestavgddahs[idx];
	    nearestmidx = idx;
	}
    }

    if ( nearestddah>maxAllowedDDah )
	return false;

    markernm = nearestmarkernms.get( nearestmidx );
    return true;
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
    BufferString tiemarkernm;
    const int newevidx = events_.size()-1;
    if ( findTieMarker(newevidx,tiemarkernm) )
    {
	const int tiedtoevidx = tiedToEventIdx( tiemarkernm );
	if ( tiedtoevidx<0 )
	    setTieMarker( newevidx, tiemarkernm );
	else
	    setNearestTieEvent( tiedtoevidx, newevidx, tiemarkernm );
    }
}


void ProfileModelFromEventData::prepareIntersectionMarkers()
{
    sortEventsonDepthIDs();
    setIntersectMarkers();
    interpolateIntersectMarkers();
    sortIntersectionMarkers();
}


void ProfileModelFromEventData::sortIntersectionMarkers()
{
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	if ( !isIntersectMarker(iev) )
	    continue;

	const BufferString topevmarkernm =
	    events_.validIdx(iev-1) ? events_[iev-1]->getMarkerName()
				    : BufferString::empty();
	const BufferString evmarkernm = events_[iev]->getMarkerName();
	const BufferString botevmarkernm =
	    events_.validIdx(iev+1) ? events_[iev+1]->getMarkerName()
				    : BufferString::empty();
	for ( int iprof=0; iprof<model_.size(); iprof++ )
	{
	    ProfileBase* prof = model_.get( iprof );
	    if ( !prof->isWell() )
		continue;

	    const int evmarkeridx = prof->markers_.indexOf( evmarkernm );
	    if ( evmarkeridx<0 )
		continue;

	    const float evmarkerdah = prof->markers_[evmarkeridx]->dah();
	    const int topevmarkeridx = prof->markers_.indexOf( topevmarkernm );
	    const int botevmarkeridx = prof->markers_.indexOf( botevmarkernm );
	    const float topevmarkerdah =
		topevmarkeridx<0 ? -mUdf(float)
				 : prof->markers_[topevmarkeridx]->dah();
	    const float botevmarkerdah =
		botevmarkeridx<0 ? mUdf(float)
				 : prof->markers_[botevmarkeridx]->dah();
	    if ( topevmarkerdah>evmarkerdah )
		prof->markers_[evmarkeridx]->setDah( topevmarkerdah );
	    else if ( botevmarkerdah<evmarkerdah )
		prof->markers_[evmarkeridx]->setDah( botevmarkerdah );
	}
    }
}


void ProfileModelFromEventData::sortEventsonDepthIDs()
{
    TypeSet<int> evdepthids, sortedevidxs;
    evdepthids.setSize( events_.size(), mUdf(int) );
    sortedevidxs.setSize( events_.size(), mUdf(int) );
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	evdepthids[iev] = events_[iev]->zvalprov_->depthID();
	sortedevidxs[iev] = iev;
    }

    sort_coupled( evdepthids.arr(), sortedevidxs.arr(), sortedevidxs.size() );
    ObjectSet<Event> sortedevents;
    for ( int idx=0; idx<evdepthids.size(); idx++ )
	sortedevents.add( events_[ sortedevidxs[idx] ] );
    events_ = sortedevents;
}


void ProfileModelFromEventData::removeAllEvents()
{
    while( events_.size() )
	removeEvent( events_.size()-1 );
}


void ProfileModelFromEventData::removeEvent( int evidx )
{
    if ( !events_.validIdx(evidx) )
	return;

    if ( isIntersectMarker(evidx) )
	model_.removeMarker( events_[evidx]->newintersectmarker_->name() );

    delete events_.removeSingle( evidx );
}


static float getDepthVal( const ProfileModelBase& model, float pos,
			  float zval )
{
    if ( !SI().zIsTime() || mIsUdf(zval) )
	return zval;

    return model.getDepthVal( zval, pos );
}


float ProfileModelFromEventData::getEventDepthVal(
	int evidx, const ProfileBase& prof ) const
{
    float evdepthval = getZValue( evidx, prof.coord_ );
    if ( mIsUdf(evdepthval) )
	return mUdf(float);

    return getDepthVal( model_, prof.pos_, evdepthval );
}


float ProfileModelFromEventData::getZOffset( int evidx,
					     const ProfileBase& prof ) const
{
    const float evdepthval = getEventDepthVal( evidx, prof );
    if ( mIsUdf(evdepthval) )
	return mUdf(float);

    if ( isIntersectMarker(evidx) )
    {
	int topevidx = evidx;
	while ( topevidx-- )
	{
	    if ( topevidx<0 || !isIntersectMarker(topevidx) )
		break;
	}

	int botevidx = evidx;
	while ( botevidx++ )
	{
	    if ( botevidx>=events_.size()-1 || !isIntersectMarker(botevidx) )
		break;
	}

	const float topzoffset =
	    events_.validIdx(topevidx) ? getZOffset( topevidx, prof ) : 0.f;
	const float topevdepthval = getEventDepthVal( topevidx, prof );
	const float botzoffset =
	    events_.validIdx(botevidx) ? getZOffset( botevidx, prof ) : 0.f;
	const float botevdepthval = getEventDepthVal( botevidx, prof );
	if ( mIsUdf(topevdepthval) )
	    return botzoffset;
	else if ( mIsUdf(botevdepthval) )
	    return topzoffset;

	const float diffdepthval = botevdepthval - topevdepthval;
	const float toprelposfac = 1 - (evdepthval-topevdepthval)/diffdepthval;
	const float botrelposfac = 1 - (botevdepthval-evdepthval)/diffdepthval;
	return toprelposfac*topzoffset + botrelposfac*botzoffset;
    }

    const Well::Marker* mrkr =
	prof.markers_.getByName( getMarkerName(evidx) );
    return mrkr && !mIsUdf(mrkr->dah()) ? mrkr->dah() - evdepthval
					: mUdf(float);
}


void ProfileModelFromEventData::setIntersectMarkers()
{
    for ( int evidx=0; evidx<nrEvents(); evidx++ )
	setIntersectMarkersForEV( model_, *events_[evidx] );
}


void ProfileModelFromEventData::setIntersectMarkersForEV(
	ProfileModelBase& model, Event& event )
{
    if ( !event.newintersectmarker_ )
	return;

    for ( int iprof=0; iprof<model.size(); iprof++ )
    {
	ProfileBase* prof = model.get( iprof );
	if ( !prof->isWell() )
	    continue;

	float dah = event.zvalprov_->getZValue( prof->coord_ );
	dah = getDepthVal( model, prof->pos_, dah );
	if ( mIsUdf(dah) )
	    continue;

	const int newtiemarkeridx =
	    prof->markers_.indexOf( event.newintersectmarker_->name() );
	if ( newtiemarkeridx>=0 )
	{
	    prof->markers_[newtiemarkeridx]->setDah( dah );
	    continue;
	}

	Well::Marker* newtiemarker =
	    new Well::Marker( event.newintersectmarker_->name() );
	*newtiemarker = *event.newintersectmarker_;
	newtiemarker->setDah( dah );
	prof->markers_.insertNew( newtiemarker );
    }
}



void ProfileModelFromEventData::interpolateIntersectMarkers()
{
    ProfilePosProviderFromLine posprov( section_.linegeom_ );
    for ( int iev=0; iev<nrEvents(); iev++ )
    {
	if ( !isIntersectMarker(iev) )
	    continue;

	interpolateIntersectMarkersForEV( model_, *events_[iev], posprov );
    }
}


void ProfileModelFromEventData::interpolateIntersectMarkersForEV(
	ProfileModelBase& model, Event& ev, const ProfilePosProvider& posprov )
{
    BufferString evmarkernm = ev.getMarkerName();
    for ( int im=0; im<model.size(); im++ )
    {
	ProfileBase* prof = model.get( im );
	const int evmarkeridx = prof->markers_.indexOf( evmarkernm );
	if ( evmarkeridx<0 )
	    continue;

	if ( !mIsUdf(prof->markers_[evmarkeridx]->dah()) )
	    continue;

	const float interpoldah =
	    getInterpolatedDepthAtPosFromEV( prof->pos_, ev, model, posprov );
	if ( mIsUdf(interpoldah) )
	{
	    pFreeFnErrMsg( "Cant extrapolate wellmarker intersection" );
	    continue;
	}

	prof->markers_[evmarkeridx]->setDah( interpoldah );
    }
}



float ProfileModelFromEventData::getInterpolatedDepthAtPosFromEV(
	float pos, const Event& event, const ProfileModelBase& model,
	const ProfilePosProvider& posprov )
{
    const float dpos = 1.f/1000.f;
    const ZValueProvider* zvalprov = event.zvalprov_;

    float prevppdmdepth = mUdf(float);
    float prevpos = pos;
    while ( prevpos -= dpos && prevpos>=0 )
    {
	const Coord prevcoord = posprov.getCoord( prevpos );
	prevppdmdepth = zvalprov->getZValue( prevcoord );
	prevppdmdepth = getDepthVal( model, prevpos, prevppdmdepth );
	if ( !mIsUdf(prevppdmdepth) )
	    break;
    }

    float nextppdmdepth = mUdf(float);
    float nextpos = pos;
    while ( nextpos += dpos && nextpos<=1.0f )
    {
	const Coord nextcoord = posprov.getCoord( nextpos );
	nextppdmdepth = zvalprov->getZValue( nextcoord );
	nextppdmdepth = getDepthVal( model, nextpos, nextppdmdepth );
	if ( !mIsUdf(nextppdmdepth) )
	    break;
    }

    if ( mIsUdf(prevppdmdepth) || mIsUdf(nextppdmdepth) )
	return mUdf(float);

    const float relpos = (pos - prevpos) / (nextpos - prevpos);
    return (prevppdmdepth*(1-relpos)) + (nextppdmdepth*relpos);
}
