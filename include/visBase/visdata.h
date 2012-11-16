#ifndef visdata_h
#define visdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "callback.h"
#include "refcount.h"
#include "sets.h"
#include "visdataman.h"

class SoNode;
class IOPar;
class BufferString;

namespace visBase { class DataObject; class EventInfo; }

namespace osg { class Node; }


#define mVisTrans visBase::Transformation


namespace visBase
{

class Transformation;
class SelectionManager;
class DataManager;
class Scene;
class DataObjectGroup;


// OSG traversal bitmasks defined by OpendTect
inline unsigned int cEventTraversalMask() 		{ return 0x00000001; }
inline unsigned int cIntersectionTraversalMask()	{ return 0x00000002; }
inline unsigned int cBBoxTraversalMask()		{ return 0x00000004; }


/*!\brief
DataObject is the base class off all objects that are used in Visualisation and
ought to be shared in visBase::DataManager. The DataManager owns all the
objects and is thus the only one that is allowed to delete it. The destructors
on the inherited classes should thus be protected.
*/

mClass(visBase) DataObject : public CallBacker
{ mRefCountImpl(DataObject);
public:

    virtual const char*		getClassName() const	{ return "Not impl"; }

    virtual bool		isOK() const		{ return true; }

    int				id() const		{ return id_; }
    void			setID(int nid);

    FixedString			name() const;
    virtual void		setName(const char*);

    osg::Node*			osgNode()		{return gtOsgNode();}
    const osg::Node*		osgNode() const
				    { return const_cast<DataObject*>(this)->
							gtOsgNode(); }

    void			enableTraversal(unsigned int mask,bool yn=true);
    bool			isTraversalEnabled(unsigned int mask) const;

    inline SoNode*		getInventorNode()	{return 0;}
    inline const SoNode*	getInventorNode() const
				{ return 0; }

    virtual bool		pickable() const 	{ return selectable(); }
    virtual bool		rightClickable() const 	{ return selectable(); }
    virtual bool		selectable() const	{ return false; }
    void			select() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    void			deSelect() const;
    				/*<! Is here for convenience. Will rewire to
				     SelectionManager.	*/
    virtual bool		isSelected() const;
    virtual NotifierAccess*	selection() 		{ return 0; }
    virtual NotifierAccess*	deSelection() 		{ return 0; }
    virtual NotifierAccess*	rightClicked()		{ return 0; }
    virtual const TypeSet<int>*	rightClickedPath() const{ return 0; }

    virtual void		setDisplayTransformation(const mVisTrans*);
    				/*!< All positions going from the outside
				     world to the vis should be transformed
				     witht this transform. This enables us
				     to have different coord-systems outside
				     OI, e.g. we can use UTM coords
				     outside the vis without loosing precision
				     in the vis.
				 */
    virtual const mVisTrans*	getDisplayTransformation() const { return 0; }
    				/*!< All positions going from the outside
				     world to the vis should be transformed
				     witht this transform. This enables us
				     to have different coord-systems outside
				     OI, e.g. we can use UTM coords
				     outside the vis without loosing precision
				     in the vis.
				 */
    virtual void		setRightHandSystem(bool yn)	{}
    				/*!<Sets whether the coordinate system is
				    right or left handed. */
    virtual bool		isRightHandSystem() const	{ return true; }

    virtual const char*		errMsg() const	{ return 0; }

    virtual bool		acceptsIncompletePar() const    {return false;}
				/*!<Returns true if it can cope with non-
				    complete session-restores. If it returns
				    false, DataManager::usePar() will
				    fail if usePar of this function returns
				    0, and it doesn't help to retry.  */

    bool			serialize(const char* filename,
	    				  bool binary=false);
    
    void			setParent(DataObjectGroup* g) { parent_ = g; }

protected:
    virtual bool		init()			{ return true; }
				//!<Called from create()

    friend class		SelectionManager;
    friend class		Scene;
    virtual void		triggerSel()				{}
    				/*!<Is called everytime object is selected.*/
    virtual void		triggerDeSel()				{}
    				/*!<Is called everytime object is deselected.*/
    virtual void		triggerRightClick(const EventInfo* =0)	{}
    
				DataObject();
    
    virtual osg::Node*		gtOsgNode()		{ return 0; }

    void			updateOsgNodeData();
    
    DataObjectGroup*		parent_;

private:
    int				id_;
    BufferString*		name_;
};

};

#define _mCreateDataObj(clss) 					\
{								\
    clss* ret = new clss;					\
    if ( !ret )							\
        return 0;						\
    if ( !ret->init() || !ret->isOK() )				\
        { ret->ref(); ret->unRef(); return 0; }			\
    return ret;							\
}								\
								\
private:							\
    static visBase::DataObject* createInternal();		\
    clss(const clss&);						\
    clss& operator =(const clss&);				\
public:								\
    static void		initClass();				\
    static const char*	getStaticClassName();			\
								\
    virtual const char*	getClassName() const; 			\
protected:							\

#define _mDeclConstr(clss)					\
    clss();							\
public:

#define mCreateDataObj(clss) 					\
    _mCreateDataObj(clss) 					\
    _mDeclConstr(clss)


#define mImplVisInitClass( clss ) \
void clss::initClass()						\
{								\
}

#define mCreateFactoryEntryNoInitClass( clss )			\
const char* clss::getStaticClassName() { return #clss; }	\
const char* clss::getClassName() const				\
{ return clss::getStaticClassName(); }

#define mCreateFactoryEntry( clss )				\
mImplVisInitClass( clss );					\
mCreateFactoryEntryNoInitClass( clss );		




#endif

