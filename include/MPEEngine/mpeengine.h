#ifndef mpeengine_h
#define mpeengine_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: mpeengine.h 38753 2015-04-11 21:19:18Z nanne.hemstra@dgbes.com $
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "attribsel.h"
#include "callback.h"
#include "datapack.h"
#include "emposid.h"
#include "survgeom.h"

class BufferStringSet;
class Executor;

namespace EM { class EMObject; }
namespace Geometry { class Element; }
template <class T> class Selector;

namespace MPE
{

class EMTracker;
class HorizonTrackerMgr;
class ObjectEditor;

/*!
\brief Main engine for tracking EM objects like horizons, faults etc.,
*/

mExpClass(MPEEngine) Engine : public CallBacker
{
    mGlobal(MPEEngine) friend Engine&		engine();

public:
			Engine();
    virtual		~Engine();

    void		init();

    const TrcKeyZSampling&	activeVolume() const;
    void		setActiveVolume(const TrcKeyZSampling&);
    Notifier<Engine>	activevolumechange;

    void		setActive2DLine(Pos::GeomID);
    Pos::GeomID 	activeGeomID() const;

    Notifier<Engine>	loadEMObject;
    MultiID		midtoload;

    void		updateSeedOnlyPropagation(bool);
    Executor*		trackInVolume();
    HorizonTrackerMgr*	trackInVolume(int idx);
    bool		trackingInProgress() const;
    void		stopTracking();
    void		removeSelectionInPolygon(const Selector<Coord3>&,
	    					 TaskRunner*);
    void		getAvailableTrackerTypes(BufferStringSet&)const;

    int			nrTrackersAlive() const;
    int			highestTrackerID() const;
    const EMTracker*	getTracker(int idx) const;
    EMTracker*		getTracker(int idx);
    int			getTrackerByObject(const EM::ObjectID&) const;
    int			getTrackerByObject(const char*) const;
    int			addTracker(EM::EMObject*);
    void		removeTracker(int idx);
    Notifier<Engine>	trackeraddremove;
    void		setActiveTracker(const EM::ObjectID&);
    void		setActiveTracker(EMTracker*);
    EMTracker*		getActiveTracker();

    			/*Attribute stuff */
    void 		setOneActiveTracker(const EMTracker*);
    void 		unsetOneActiveTracker();
    void		getNeededAttribs(
				TypeSet<Attrib::SelSpec>&) const;
    TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const;
    			/*!< Returns the cube that is needed for
			     this attrib, given that the activearea
			     should be tracked. */
    int			getCacheIndexOf(const Attrib::SelSpec&) const;
    DataPack::ID	getAttribCacheID(const Attrib::SelSpec&) const;
    bool		hasAttribCache(const Attrib::SelSpec&) const;
    bool		setAttribData( const Attrib::SelSpec&,
	    			       DataPack::ID);
    bool		cacheIncludes(const Attrib::SelSpec&,
				      const TrcKeyZSampling&);
    void		swapCacheAndItsBackup();

    bool		isSelSpecSame(const Attrib::SelSpec& setupss,
	    			      const Attrib::SelSpec& clickedss) const;

    void		updateFlatCubesContainer(const TrcKeyZSampling& tkzs,
	    					 const int idx,bool);
			/*!< add = true, remove = false. */
    ObjectSet<TrcKeyZSampling>* getTrackedFlatCubes(const int idx) const;
    DataPack::ID	getSeedPosDataPack(const TrcKey&,float z,int nrtrcs,
					const StepInterval<float>& zrg) const;

    			/*Editors */
    ObjectEditor*	getEditor(const EM::ObjectID&,bool create);
    void		removeEditor(const EM::ObjectID&);

    const char*		errMsg() const;

    BufferString	setupFileName( const MultiID& ) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    BufferString		errmsg_;
    TrcKeyZSampling		activevolume_;

    Pos::GeomID 		activegeomid_;

    ObjectSet<HorizonTrackerMgr> trackermgrs_;
    ObjectSet<EMTracker>	trackers_;
    ObjectSet<ObjectEditor>	editors_;

    const EMTracker*		oneactivetracker_;
    EMTracker*			activetracker_;
    DataPackMgr&		dpm_;

    struct CacheSpecs
    {
				CacheSpecs(const Attrib::SelSpec& as,
					Pos::GeomID geomid=
					Survey::GeometryManager::cUndefGeomID())
				    : attrsel_(as),geomid_(geomid)
				{}

	Attrib::SelSpec		attrsel_;
	Pos::GeomID		geomid_;
    };

    TypeSet<DataPack::ID>	attribcachedatapackids_;
    ObjectSet<CacheSpecs>	attribcachespecs_;
    TypeSet<DataPack::ID>	attribbkpcachedatapackids_;
    ObjectSet<CacheSpecs>	attribbackupcachespecs_;

    mStruct(MPEEngine) FlatCubeInfo
    {
				FlatCubeInfo()
				:nrseeds_(1)
				{
				    flatcs_.setEmpty();
				}
	TrcKeyZSampling		flatcs_;
	int			nrseeds_;
    };

    ObjectSet<ObjectSet<FlatCubeInfo> >	flatcubescontainer_;

    static const char*		sKeyNrTrackers(){ return "Nr Trackers"; }
    static const char*		sKeyObjectID()	{ return "ObjectID"; }
    static const char*		sKeyEnabled()	{ return "Is enabled"; }
    static const char*		sKeyTrackPlane(){ return "Track Plane"; }
    static const char*		sKeySeedConMode(){ return "Seed Connect Mode"; }
};


mGlobal(MPEEngine) Engine&	engine();

} // namespace MPE

#endif
