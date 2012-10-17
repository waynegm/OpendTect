#ifndef visobject_h
#define visobject_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visdata.h"
#include "thread.h"
#include "visosg.h"

class Coord3;

namespace osg { class Switch; class StateSet; }

namespace visBase
{
class Material;
class Transformation;
class EventCatcher;
class NodeState;

/*!\brief Base class for all objects that are visual on the scene. */

mClass(visBase) VisualObject : public DataObject
{
public:
    virtual void		turnOn(bool)				= 0;
    virtual bool		isOn() const				= 0;

    virtual void		setMaterial( Material* )		= 0;
    virtual Material*		getMaterial()				= 0;

    virtual bool		getBoundingBox(Coord3& min,Coord3& max) const;
    virtual void		setSceneEventCatcher( EventCatcher* )	{}

    void			setSelectable(bool yn)	{ isselectable=yn; }
    bool			selectable() const 	{ return isselectable; }
    NotifierAccess*		selection() 		{ return &selnotifier; }
    NotifierAccess*		deSelection() 		{return &deselnotifier;}
    virtual NotifierAccess*	rightClicked()		{ return &rightClick; }
    const EventInfo*		rightClickedEventInfo() const{return rcevinfo;}
    const TypeSet<int>*		rightClickedPath() const;
    
    template <class T> T*	addNodeState(T* ns)
				{ doAddNodeState(ns); return ns; }
    NodeState*			removeNodeState(NodeState*);

protected:
    void			doAddNodeState(NodeState* ns);
    virtual osg::StateSet*	getStateSet();
    
    void			triggerSel()
    				{ if (isselectable) selnotifier.trigger(); }
    void			triggerDeSel()
    				{ if (isselectable) deselnotifier.trigger(); }
    void			triggerRightClick(const EventInfo*);
				VisualObject(bool selectable=false);
				~VisualObject();

private:
    
    ObjectSet<NodeState>	nodestates_;
    bool			isselectable;
    Notifier<VisualObject>	selnotifier;
    Notifier<VisualObject>	deselnotifier;
    Notifier<VisualObject>	rightClick;
    const EventInfo*		rcevinfo;
};


mClass(visBase) VisualObjectImpl : public VisualObject
{
public:

    void		turnOn(bool);
    bool		isOn() const;
    void		removeSwitch();
    void		setRightHandSystem(bool yn) { righthandsystem_=yn; }
    bool		isRightHandSystem() const { return righthandsystem_; }

    void		setLockable();
    			/*!<Will enable lock functionality.
			   \note Must be done before giving away the SoNode with
			   getInventorNode() to take effect */
    void		readLock();
    void		readUnLock();
    bool		tryReadLock();
    void		writeLock();
    void		writeUnLock();
    bool		tryWriteLock();

    void		setMaterial(Material*);
    const Material*	getMaterial() const { return material_; }
    Material*		getMaterial();

    static const char*	sKeyMaterialID(); //Remove
    static const char*	sKeyMaterial();
    static const char*	sKeyIsOn();

    virtual int		usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

protected:
    
    void		addChild(SoNode*);
    void		insertChild(int pos,SoNode*);
    void		removeChild(SoNode*);
    int			childIndex(const SoNode*) const;
    SoNode*		getChild(int);

    int			addChild(osg::Node*);
			//!<\returns new child index
    void		insertChild(int pos,osg::Node*);
    void		removeChild(osg::Node*);
    int			childIndex(const osg::Node*) const;


			VisualObjectImpl(bool selectable);
    virtual		~VisualObjectImpl();

    Material*		material_;
    bool		righthandsystem_;

    osg::Node*		gtOsgNode();

private:
    
    OsgRefMan<osg::Switch>	osgroot_;
};

mLockerClassImpl( visBase, VisualReadLockLocker, VisualObjectImpl,
		  readLock(), readUnLock(), tryReadLock() )
mLockerClassImpl( visBase, VisualWriteLockLocker, VisualObjectImpl,
		  writeLock(), writeUnLock(), tryWriteLock() )
};


/*!\mainpage Visualisation - Open Inventor-based tools

  All 3D visualisation in OpendTect is COIN based. COIN is an implementation
  of the OpenInventor interface sepecification. As usual, the external package
  (i.e. COIN) is not visible to any object outside this module.
  This module can be seen as a layer on top of the COIN library. Compliant with
  the 'DIF' principle (Don't implement the future), the layer only contains
  those tools that are really used in OpendTect.

  The function initODInventorClasses() initialises our own 
  Inventor classes, which are in fact extensions of the COIN library. 
  These classes can be found is SoOD. The 'vis' classes in this module are 
  used in the rest of OpendTect. 

  In the 3D visualisation world you'll see that it is almost unavoidable that
  all objects will be managed by a centralised manager. That is
  DataManager. 

  The main classes for displaying scene objects are:
  - TextureRect, for inlines, crosslines and timeslices.
  - RandomTrack, for random lines.
  - CubeView, for the volume viewer.
  - Marker, for picks.
  - HorizonSection, for Horizons.
  - Well, for wells.

*/

#endif

