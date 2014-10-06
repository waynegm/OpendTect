#ifndef uiodemsurftreeitem_h
#define uiodemsurftreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uiodattribtreeitem.h"
#include "uioddisplaytreeitem.h"
#include "emposid.h"

class DataPointSet;
class uiVisEMObject;
class uiODDataTreeItem;


mExpClass(uiODMain) uiODEarthModelSurfaceTreeItem : public uiODDisplayTreeItem
{
public:

    uiVisEMObject*	visEMObject() const	{ return uivisemobj_; }
    EM::ObjectID	emObjectID() const	{ return emid_; }
    int			reloadEMObject();	//Return new display id.

protected:
			uiODEarthModelSurfaceTreeItem(const EM::ObjectID&);
			~uiODEarthModelSurfaceTreeItem();

    virtual void	createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);

    uiODDataTreeItem*	createAttribItem(const Attrib::SelSpec*) const;

    void		finishedEditingCB(CallBacker*);
    void		prepareForShutdown();

    void		askSaveCB(CallBacker*);
    void		saveCB(CallBacker*);

    EM::ObjectID	emid_;
    uiVisEMObject*	uivisemobj_;

    MenuItem		createflatscenemnuitem_;

protected:
    bool		init();
    virtual void	initNotify() {};
    bool		createUiVisObj();

    virtual void	checkCB(CallBacker*);
    void		selChg(CallBacker*);

    void		updateTrackingState();
    bool		istrackingallowed_;

    MenuItem		savemnuitem_;
    MenuItem		saveasmnuitem_;
    MenuItem		enabletrackingmnuitem_;
    MenuItem		changesetupmnuitem_;
    MenuItem		reloadmnuitem_;
    MenuItem		trackmenuitem_;
    MenuItem		starttrackmnuitem_;
};


mExpClass(uiODMain) uiODEarthModelSurfaceDataTreeItem :
						public uiODAttribTreeItem
{
public:
			uiODEarthModelSurfaceDataTreeItem(EM::ObjectID,
				       uiVisEMObject*,const char* parenttype);

    void		setDataPointSet(const DataPointSet&);
    void		selectAndLoadAuxData();

protected:
    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    BufferString	createDisplayName() const;

    MenuItem		depthattribmnuitem_;
    MenuItem		savesurfacedatamnuitem_;
    MenuItem		loadsurfacedatamnuitem_;
    MenuItem		algomnuitem_;
    MenuItem		fillholesmnuitem_;
    MenuItem		filtermnuitem_;
    MenuItem		horvariogrammnuitem_;
    MenuItem		attr2geommnuitm_;

    bool		changed_;
    EM::ObjectID	emid_;
    uiVisEMObject*	uivisemobj_;
};


#endif

