/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : April 2017
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "profilemodelfromeventdata.h"

#include "bendpointfinder.h"
#include "profilebase.h"
#include "profileposprovider.h"
#include "polylinend.h"
#include "randomlinegeom.h"
#include "statruncalc.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "uistrings.h"
#include "wellman.h"
#include "welldata.h"
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


bool ProfileModelFromEventData::Section::getSectionTKS(
	TrcKeySampling& sectiontks ) const
{
    sectiontks.init( false );
    if ( is2d_ )
    {
	sectiontks.set2DDef();
	const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
	if ( !geom )
	    return false;

	sectiontks = geom->sampling().hsamp_;
    }
    else
    {
	if ( linegeom_.isEmpty() )
	{
	    Geometry::RandomLine* rdlgeom = Geometry::RLM().get( rdmlinemid_ );
	    if ( !rdlgeom )
		return false;

	    TrcKeyPath linegeom;
	    rdlgeom->allNodePositions( linegeom );
	    for ( int ipos=0; ipos<linegeom.size(); ipos++ )
		sectiontks.include( linegeom[ipos] );
	}
	else
	{
	    for ( int ipos=0; ipos<linegeom_.size(); ipos++ )
		sectiontks.include( SI().transform(linegeom_[ipos]) );
	}
    }

    return true;
}


bool ProfileModelFromEventData::Section::fetchLineGeom()
{
    return is2d_ ? fetchLineGeom2D() : fetchLineGeom3D();
}


bool ProfileModelFromEventData::Section::fetchLineGeom2D()
{
    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom)
    if ( !geom2d )
    {
	errmsg_ = uiStrings::phrCannotRead(
		tr("the line's geometry from the database") );
	return false;
    }

    TypeSet<Coord> coords;
    const PosInfo::Line2DData& l2dd = geom2d->data();
    for ( int idx=0; idx<l2dd.positions().size(); idx++ )
	coords += l2dd.positions()[idx].coord_;

    BendPointFinder2D bpfndr( coords, 1 );
    bpfndr.execute();
    for ( int idx=0; idx<bpfndr.bendPoints().size(); idx++ )
	linegeom_ += l2dd.positions()[ bpfndr.bendPoints()[idx] ].coord_;

    return true;
}


bool ProfileModelFromEventData::Section::fetchLineGeom3D()
{
    Geometry::RandomLine* rdlgeom = Geometry::RLM().get( rdmlinemid_ );
    if ( !rdlgeom )
    {
	errmsg_ = uiStrings::phrCannotRead(
		tr("the randome line's geometry from the database") );
	return false;
    }

    for ( int idx=0; idx<rdlgeom->nrNodes(); idx++ )
	linegeom_ += SI().transform( rdlgeom->nodePosition( idx ) );

    if ( linegeom_.size() < 2 )
    {
	errmsg_ = tr( "Less than 2 points in random line" );
	return false;
    }

    return true;
}


void ProfileModelFromEventData::Section::fillPar( IOPar& par ) const
{
    par.setYN( sKey::TwoD(), is2d_ );
    if ( is2d_ )
	par.set( sKey::GeomID(), geomid_ );
    else
	par.set( sKeyRandomLineID(), rdmlinemid_ );
    par.set( sKeySeisID(), seismid_ );
}


void ProfileModelFromEventData::Section::usePar( const IOPar& par )
{
    par.getYN( sKey::TwoD(), is2d_ );
    if ( is2d_ )
	par.get( sKey::GeomID(), geomid_ );
    else
	par.get( sKeyRandomLineID(), rdmlinemid_ );
    par.get( sKeySeisID(), seismid_ );
}


ProfileModelFromEventData::Event::Event( ZValueProvider* zprov )
    : zvalprov_(zprov)
    , newintersectmarker_(0)
{
    setMarker( zprov->getName().getFullString() );
}


ProfileModelFromEventData::Event::~Event()
{
    delete zvalprov_;
    if ( newintersectmarker_ )
	Strat::eLVLS().remove( levelid_ );
    delete newintersectmarker_;
}


void ProfileModelFromEventData::Event::fillPar( IOPar& par ) const
{
    zvalprov_->fillPar( par );
    par.set( sKeyMarkerName(), getMarkerName() );
}


ProfileModelFromEventData::Event* ProfileModelFromEventData::Event::
	createNewEvent( const IOPar& par, const TrcKeySampling& tks,
			TaskRunner* taskrunner )
{
    BufferString keystr;
    if ( !par.get(ZValueProvider::sType(),keystr) )
	return 0;

    ZValueProvider* zvalprov =
	ZValueProvider::factory().create( keystr, par, tks, taskrunner );
    if ( !zvalprov )
	return 0;

    Event* newevent = new Event( zvalprov );
    BufferString tiemarkernm;
    par.get( sKeyMarkerName(), tiemarkernm );
    newevent->setMarker( tiemarkernm );
    return newevent;
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


BufferString ProfileModelFromEventData::Event::getMarkerName() const
{
    return newintersectmarker_ ? newintersectmarker_->name() : tiemarkernm_;
}

static int sDefNrCtrlProfiles = 50;

ProfileModelFromEventData::ProfileModelFromEventData(
	ProfileModelBase* model, const TypeSet<Coord>& linegeom )
    : model_(model)
    , section_(linegeom)
    , totalnrprofs_(sDefNrCtrlProfiles)
{
}


ProfileModelFromEventData::ProfileModelFromEventData(
	ProfileModelBase* model )
    : model_(model)
    , totalnrprofs_(sDefNrCtrlProfiles)
{
}


ProfileModelFromEventData::~ProfileModelFromEventData()
{
    removeAllEvents();
}


bool ProfileModelFromEventData::hasPar( const IOPar& par )
{
    PtrMan<IOPar> proffromevdatapar = par.subselect( sKeyStr() );
    return proffromevdatapar;
}


void ProfileModelFromEventData::fillPar( IOPar& par ) const
{
    IOPar proffromevpar;
    IOPar sectionpar;
    proffromevpar.set( sKeyNrProfs(), totalnrprofs_ );
    proffromevpar.set( sKeyEventType(), eventtypestr_ );
    section_.fillPar( sectionpar );
    proffromevpar.mergeComp( sectionpar, sKeySection() );
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	IOPar eventpar;
	events_[iev]->fillPar( eventpar );
	proffromevpar.mergeComp( eventpar, IOPar::compKey(sKeyEvent(),iev) );
    }

    par.mergeComp( proffromevpar, sKeyStr() );
}


ProfileModelFromEventData* ProfileModelFromEventData::createFrom(
	ProfileModelBase& model, const IOPar& par, TaskRunner* taskrunner )
{
    PtrMan<IOPar> proffromevpar = par.subselect( sKeyStr() );
    if ( !proffromevpar )
	return 0;

    BufferString eventtypestr;
    if ( !proffromevpar->get(sKeyEventType(),eventtypestr) )
	return 0;

    ProfileModelFromEventData* profmodelfromdata =
	new ProfileModelFromEventData( &model );
    profmodelfromdata->eventtypestr_ = eventtypestr;
    proffromevpar->get( sKeyNrProfs(), profmodelfromdata->totalnrprofs_ );
    PtrMan<IOPar> sectionpar = proffromevpar->subselect( sKeySection() );
    profmodelfromdata->section_.usePar( *sectionpar );
    TrcKeySampling sectiontks;
    profmodelfromdata->section_.fetchLineGeom();
    profmodelfromdata->section_.getSectionTKS( sectiontks );
    int iev = 0;
    while( true )
    {
	PtrMan<IOPar> eventpar =
	    proffromevpar->subselect( IOPar::compKey(sKeyEvent(),iev) );
	if ( !eventpar )
	    break;

	iev++;
	profmodelfromdata->events_ +=
	    Event::createNewEvent( *eventpar, sectiontks, taskrunner );
    }

    profmodelfromdata->prepareIntersectionMarkers();
    model.regenerateWells();
    return profmodelfromdata;
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
	model_->removeMarker( ev.newintersectmarker_->name() );

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
    for ( int iprof=0; iprof<model_->size(); iprof++ )
    {
	const ProfileBase* prof = model_->get( iprof );
	if ( !prof->isWell() )
	    continue;

	const float evdah = getEventDepthVal( evidx, *prof );
	const int markeridx = prof->markers_.indexOf( markernm );
	if ( markeridx<0 )
	    continue;

	statrc.addValue( prof->markers_[markeridx]->dah()-evdah );
    }

    return fabs( mCast(float,statrc.average()) );
}


bool ProfileModelFromEventData::findTieMarker( int evidx,
					       BufferString& markernm ) const
{
    if ( !isIntersectMarker(evidx) )
	return true;

    BufferStringSet nearestmarkernms;
    for ( int iprof=0; iprof<model_->size(); iprof++ )
    {
	const ProfileBase* prof = model_->get( iprof );
	if ( !prof->isWell() )
	    continue;

	const float evdah = getEventDepthVal( evidx, *prof );
	if ( mIsUdf(evdah) )
	    continue;

	const int topmarkeridx = prof->markers_.getIdxAbove( evdah - 0.1 );
	const int botmarkeridx = prof->markers_.getIdxBelow( evdah + 0.1 );
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
	for ( int iprof=0; iprof<model_->size(); iprof++ )
	{
	    const ProfileBase* prof = model_->get( iprof );
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

	nearestavgddahs[idx] = fabs( mCast(float,statrc.average()) );
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


bool ProfileModelFromEventData::hasIntersectMarker() const
{
    for ( int iev=0; iev<nrEvents(); iev++ )
    {
	if ( isIntersectMarker(iev) )
	    return true;
    }

    return false;
}


void ProfileModelFromEventData::prepareIntersectionMarkers()
{
    model_->removeProfiles();
    for ( int iev=0; iev<events_.size(); iev++ )
    {
	if ( isIntersectMarker(iev) )
	    removeMarkers( events_[iev]->getMarkerName() );
    }
    if ( !hasIntersectMarker() )
	return;

    setIntersectMarkers();
    removeCrossingEvents();
}


bool ProfileModelFromEventData::getTopBottomMarker(
	const Event& event, const ProfileBase& prof, BufferString& markernm,
	bool istop ) const
{
    const BufferString evmarkernm = event.getMarkerName();
    const int evmarkeridx = prof.markers_.indexOf( evmarkernm );
    if ( evmarkeridx<0 )
	return false;

    const int incr = istop ? -1 : +1;
    const int topbotmarkeridx =
	prof.markers_.validIdx(evmarkeridx+incr) ? evmarkeridx+incr : -1;
    if ( !prof.markers_.validIdx(topbotmarkeridx) )
	return false;

    markernm = prof.markers_[topbotmarkeridx]->name();
    return true;
}


bool ProfileModelFromEventData::isEventCrossing( const Event& event ) const
{
    for ( int iprof=0; iprof<model_->size()-1; iprof++ )
    {
	const ProfileBase* curprof = model_->get( iprof );
	const ProfileBase* nextprof = model_->get( iprof+1 );
	BufferString curtopmarker, curbotmarker;
	getTopBottomMarker( event, *curprof, curtopmarker, true );
	getTopBottomMarker( event, *curprof, curbotmarker, false );
	const Well::Marker* evmarkerinnext =
	    nextprof->markers_.getByName( event.getMarkerName() );
	if ( !evmarkerinnext )
	    continue;

	const Well::Marker* curproftopmarkerinnext =
	    nextprof->markers_.getByName( curtopmarker );
	int curprotopmarkeridx = curprof->markers_.indexOf( curtopmarker );
	while ( !curproftopmarkerinnext )
	{
	    curprotopmarkeridx--;
	    if ( curprotopmarkeridx<0 )
		break;

	    curtopmarker = curprof->markers_[curprotopmarkeridx]->name();
	    curproftopmarkerinnext = nextprof->markers_.getByName(curtopmarker);
	}

	if ( curproftopmarkerinnext &&
	     curproftopmarkerinnext->dah()>evmarkerinnext->dah() )
	    return true;

	const Well::Marker* curprofbotmarkerinnext =
	    nextprof->markers_.getByName( curbotmarker );
	int curprobotmarkeridx = curprof->markers_.indexOf( curbotmarker );
	while ( !curprofbotmarkerinnext )
	{
	    curprobotmarkeridx++;
	    if ( curprobotmarkeridx>=curprof->markers_.size() )
		break;

	    curbotmarker = curprof->markers_[curprobotmarkeridx]->name();
	    curprofbotmarkerinnext = nextprof->markers_.getByName(curbotmarker);
	}
	if ( curprofbotmarkerinnext &&
	     curprofbotmarkerinnext->dah()<evmarkerinnext->dah() )
	    return true;
    }

    return false;
}


void ProfileModelFromEventData::removeCrossingEvents()
{
    uiStringSet removedevents;
    for ( int iev=events_.size()-1; iev>=0; iev-- )
    {
	if ( isEventCrossing(*events_[iev]) )
	{
	    removedevents += events_[iev]->zvalprov_->getName();
	    removeEvent( iev );
	}
    }

    if ( !removedevents.isEmpty() )
	warnmsg_ = tr( "Following events are removed as they were crossing "
			"other events : %1" )
			.arg(removedevents.createOptionString());
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


void ProfileModelFromEventData::removeMarkers( const char* markernm )
{
    model_->removeMarker( markernm );
    for ( int idx=0; idx<model_->size(); idx++ )
    {
	const ProfileBase* prof = model_->get( idx );
	if ( !prof->isWell() )
	    continue;

	Well::Data* wd = Well::MGR().get( prof->wellid_ );
	const int markeridx = wd->markers().indexOf( markernm );
	if ( markeridx>=0 )
	    wd->markers().removeSingle( markeridx );
    }
}


void ProfileModelFromEventData::removeEvent( int evidx )
{
    if ( !events_.validIdx(evidx) )
	return;

    if ( isIntersectMarker(evidx) )
	removeMarkers( events_[evidx]->newintersectmarker_->name() );

    delete events_.removeSingle( evidx );
}


static float getDepthVal( const ProfileModelBase& model, float pos,
			  float zval, bool depthintvdss )
{
    if ( !SI().zIsTime() || mIsUdf(zval) )
	return zval;

    return model.getDepthVal( zval, pos, depthintvdss );
}


float ProfileModelFromEventData::getEventDepthVal(
	int evidx, const ProfileBase& prof, bool depthintvdss ) const
{
    float evdepthval = getZValue( evidx, prof.coord_ );
    if ( mIsUdf(evdepthval) )
    {
	ProfilePosProviderFromLine posprov( section_.linegeom_ );
	return getInterpolatedDepthAtPosFromEV( prof.pos_,*events_[evidx],
						*model_, posprov, depthintvdss);
    }

    float depthval = getDepthVal( *model_, prof.pos_, evdepthval, depthintvdss);
    return depthval;
}


float ProfileModelFromEventData::calcZOffsetForIntersection(
	int evidx, const ProfileBase& prof ) const
{
    if ( !isIntersectMarker(evidx) )
	return 0.0f;

    const float evdepthval = getEventDepthVal( evidx, prof );
    if ( mIsUdf(evdepthval) )
	return 0.0f;

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


float ProfileModelFromEventData::getZOffset( int evidx,
					     const ProfileBase& prof ) const
{
    const float evdepthval = getEventDepthVal( evidx, prof );
    if ( mIsUdf(evdepthval) )
	return mUdf(float);

    const Well::Marker* mrkr =
	prof.markers_.getByName( getMarkerName(evidx) );
    return mrkr && !mIsUdf(mrkr->dah()) ? mrkr->dah() - evdepthval
					: mUdf(float);
}


void ProfileModelFromEventData::setIntersectMarkers()
{
    for ( int evidx=0; evidx<nrEvents(); evidx++ )
	setIntersectMarkersForEV( evidx );
}


void setMarker( const Well::Marker& marker, float depth, ProfileBase* prof,
		bool setinwell )
{
    Well::MarkerSet& markers =
	setinwell ? Well::MGR().get(prof->wellid_)->markers() : prof->markers_;
    const int markeridx = markers.indexOf( marker.name() );
    if ( markeridx>=0 )
	markers[markeridx]->setDah( depth );
    else
    {
	Well::Marker* newtiemarker = new Well::Marker( marker.name() );
	newtiemarker->setColor( marker.color() );
	newtiemarker->setDah( depth );
	markers.insertNew( newtiemarker );
    }
}


void ProfileModelFromEventData::setIntersectMarkersForEV( int evidx )
{
    Event& event = *events_[evidx];
    if ( !event.newintersectmarker_ )
	return;

    for ( int iprof=0; iprof<model_->size(); iprof++ )
    {
	ProfileBase* prof = model_->get( iprof );
	if ( !prof->isWell() )
	    continue;

	float tvdss = getEventDepthVal( evidx, *prof, true );
	if ( mIsUdf(tvdss) )
	    continue;
	setMarker( *event.newintersectmarker_, tvdss, prof, false);
	float dah = getEventDepthVal( evidx, *prof, false );
	setMarker( *event.newintersectmarker_, dah, prof, true );
    }
}


float ProfileModelFromEventData::getInterpolatedDepthAtPosFromEV(
	float pos, const Event& event, const ProfileModelBase& model,
	const ProfilePosProvider& posprov, bool depthintvdss )
{
    const float dpos = 1.f/1000.f;
    const ZValueProvider* zvalprov = event.zvalprov_;

    float prevppdmdepth = mUdf(float);
    float prevpos = pos;
    while ( prevpos>=0 )
    {
	prevpos -= dpos;
	const Coord prevcoord = posprov.getCoord( prevpos );
	prevppdmdepth = zvalprov->getZValue( prevcoord );
	prevppdmdepth = getDepthVal( model, prevpos, prevppdmdepth,
				     depthintvdss );
	if ( !mIsUdf(prevppdmdepth) )
	    break;
    }

    float nextppdmdepth = mUdf(float);
    float nextpos = pos;
    while ( nextpos<=1.0f )
    {
	nextpos += dpos;
	const Coord nextcoord = posprov.getCoord( nextpos );
	nextppdmdepth = zvalprov->getZValue( nextcoord );
	nextppdmdepth = getDepthVal( model, nextpos, nextppdmdepth,
				     depthintvdss );
	if ( !mIsUdf(nextppdmdepth) )
	    break;
    }

    if ( mIsUdf(prevppdmdepth) || mIsUdf(nextppdmdepth) )
	return mUdf(float);

    const float relpos = (pos - prevpos) / (nextpos - prevpos);
    return (prevppdmdepth*(1-relpos)) + (nextppdmdepth*relpos);
}
