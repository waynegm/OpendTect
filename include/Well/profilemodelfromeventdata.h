#ifndef profilemodelfromeventdata_h
#define profilemodelfromeventdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Feb 2017
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "bufstringset.h"
#include "coord.h"
#include "datapackbase.h"
#include "refcount.h"
#include "stratlevel.h"
#include "zaxistransform.h"

class ProfileBase;
class ProfileModelBase;
class ProfilePosProviderFromLine;
class TaskRunner;
class TrcKeySampling;
class ZValueProvider;
namespace Well { class Marker; }


mExpClass(Well) ProfileModelFromEventData
{ mRefCountImpl(ProfileModelFromEventData);
mODTextTranslationClass(ProfileModelFromEventData);
public:
				ProfileModelFromEventData(
				    ProfileModelBase*,const TypeSet<Coord>&);
				ProfileModelFromEventData(
				    ProfileModelBase*);
    mStruct(Well) Section
    {
					Section( const TypeSet<Coord>& g )
					    : linegeom_(g)
					    , profposprov_(0)
					    , seisfdp_(0)	{}
					Section()
					    : seisfdp_(0)
					    , profposprov_(0)	{}
	bool				is2d_;
	Pos::GeomID			geomid_;
	MultiID				rdmlinemid_;
	MultiID				seismid_;
	ConstDataPackRef<FlatDataPack>	seisfdp_;
	TypeSet<Coord>			linegeom_;
	ProfilePosProviderFromLine*	profposprov_;

	bool				getSectionTKS(TrcKeySampling&) const;
	void				fillPar(IOPar&) const;
	void				usePar(const IOPar&);
	bool				fetchLineGeom();
	bool				fetchLineGeom2D();
	bool				fetchLineGeom3D();
	bool				isOK() const {return errmsg_.isEmpty();}
	uiString			errmsg_;
	static const char*		sKeyRandomLineID()
					{ return "RandomLineID"; }
	static const char*		sKeySeisID()
					{ return "SeisDataID"; }

    };

    mStruct(Well) Event
    {
					Event(ZValueProvider*);
					~Event();
	BufferString			tiemarkernm_;
	ZValueProvider*			zvalprov_;
	Well::Marker*			newintersectmarker_;
	void				fillPar(IOPar&) const;
	static Event*			createNewEvent(const IOPar&,
						       const TrcKeySampling&,
						       TaskRunner*);
	BufferString			getMarkerName() const;
	Color				getMarkerColor() const;
	void				setMarker(const char*);
	Strat::Level::ID		levelid_;
	static const char*		sKeyMarkerName() {return "Marker Name";}
    };

    int					totalnrprofs_;
    BufferString			eventtypestr_;
    uiString				warnmsg_;
    ProfileModelBase*			model_;
    Section				section_;
    ObjectSet<Event>			events_;
    RefMan<ZAxisTransform>		ztransform_;
    int					voiidx_;

    void				setModel( ProfileModelBase* model )
					{ model_ = model; }
    void				setTransform(ZAxisTransform*);
    bool				prepareSectionGeom();
    bool				prepareTransform();
    void				fillPar(IOPar&) const;
    static bool				hasPar(const IOPar&);
    static ProfileModelFromEventData*	createFrom(ProfileModelBase&,
						   const IOPar&,TaskRunner*);
    int					nrEvents() const
					{ return events_.size(); }
    bool				hasIntersectMarker() const;
    bool				isIntersectMarker(const char*) const;
    bool				isIntersectMarker(int evidx) const;
    float				getZValue(int evidx,const Coord&) const;
    int					tiedToEventIdx(const char* mnm) const;
    void				sortEventsonDepthIDs();
    bool				isEventCrossing(const Event&) const;
    void				removeCrossingEvents();
    void				setTieMarker(int evidx,
						const char* mnm=0 );
    void				findAndSetTieMarkers();
    bool				findTieMarker(int evidx,
						      BufferString& mnm) const;
    void				setNearestTieEvent(
						int ev1idx,int ev2idx,
						const BufferString&);
    bool				getTopBottomMarker(
					    const Event&,const ProfileBase&,
					    BufferString&,bool istop) const;
    float				getAvgDZval(int evidx,
						    const BufferString&) const;
    BufferString			getMarkerName(int evidx) const;
    void				removeMarkers(const char*);
    Well::Marker*			getIntersectMarker(int evidx) const;
    void				addEvent(ZValueProvider*);
    void				removeAllEvents();
    void				removeEvent(int evidx);
    float				getZOffset(int evidx,
						   const ProfileBase&) const;
    float				calcZOffsetForIntersection(
					    int evidx,const ProfileBase&) const;
    void				prepareIntersectionMarkers();
    void				setIntersectMarkers();
    void				setIntersectMarkersForEV(int evidx);
    float				getEventDepthVal(
						int evidx,const ProfileBase&,
						bool depthintvdss=true) const;
    float				getDepthVal(float pos,float zval,
						    bool depthintvdss) const;
    float				getInterpolatedDepthAtPosFromEV(
					    float pos, const Event&,
					    bool depthintvdss=true) const;

    static const char*			sKeyNrProfs()
					{ return "Nr of Profiles"; }
    static const char*			sKeySection()	{ return "Section"; }
    static const char*			sKeyEvent()	{ return "Event"; }
    static const char*			sKeyEventType() { return "Event Type"; }
    static const char*			sKeyStr()
					{ return "Profile from events data"; }
    static const char*			addMarkerStr()
					{return "<add marker at intersection>";}
};

#endif
