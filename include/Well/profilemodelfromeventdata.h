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
#include "datapack.h"
#include "stratlevel.h"

class ProfileModelBase;
class ZValueProvider;
namespace Well { class Marker; }


mStruct(Well) ProfileModelFromEventData
{
				ProfileModelFromEventData(
				    ProfileModelBase&,const TypeSet<Coord>&);
				~ProfileModelFromEventData();

    mStruct(Well) Section
    {
					Section( const TypeSet<Coord>& g )
					    : linegeom_(g)
					    , seisfdpid_(DataPack::cNoID()) {}
	bool				is2d_;
	Pos::GeomID			geomid_;
	MultiID				rdmlinemid_;
	DataPack::ID			seisfdpid_;
	TypeSet<Coord>			linegeom_;
    };

    mStruct(Well) Event
    {
					Event(ZValueProvider*);
					~Event();
	BufferString			tiemarkernm_;
	ZValueProvider*			zvalprov_;
	Well::Marker*			newintersectmarker_;
	BufferString			getMarkerName() const;
	void				setMarker(const char*);
	Strat::Level::ID		levelid_;
    };

    int					totalnrprofs_;
    ProfileModelBase&			model_;
    Section				section_;
    ObjectSet<Event>			events_;

    int					nrEvents() const
					{ return events_.size(); }
    bool				isIntersectMarker(const char*) const;
    bool				isIntersectMarker(int evidx) const;
    float				getZValue(int evidx,const Coord&) const;
    void				setTieMarker(int evidx,
						const char* mnm=0 );
    BufferString			getMarkerName(int evidx) const;
    Well::Marker*			getIntersectMarker(int evidx) const;
    void				addEvent(ZValueProvider*);
    void				removeEvent(int evidx);

    static const char*			dontUseStr()	{ return "<don't use>";}
    static const char*			addMarkerStr()
					{return "<add marker at intersection>";}
};

#endif
