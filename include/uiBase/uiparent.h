#ifndef uiparent_h
#define uiparent_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2001
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uilayout.h"

class MouseCursor;
class uiObjectBody;
class uiObject;
class uiMainWin;


mExpClass(uiBase) uiParent 
{
public:
				uiParent(const char* nm);

    void			addChild(uiObject&);
    const ObjectSet<uiObject>*	childList() const;
    void			attachChildren(const uiObject*,constraintType,
					       const uiObject*);

    virtual uiObject*		getUiObject()			{ return 0; }
    virtual mQtclass(QWidget)*	getParentWidget()		{ return 0; }
    const mQtclass(QWidget)*	getParentWidget() const;


    virtual uiMainWin*	mainwin()				{ return 0; }

    const uiLayoutMgr*		getLayoutMgr() const;
    virtual uiLayoutMgr*	getLayoutMgr()			{ return 0; }

    //Temporary, not sure what to do with these.
    int	 /* refnr */		beginCmdRecEvent(const char* msg=0){ return 0; }
    void			endCmdRecEvent(int refnr,const char* msg=0) {}

    int	 /* refnr */		beginCmdRecEvent(od_uint64 id,
						 const char* msg=0) { return 0; }
    void			endCmdRecEvent(od_uint64 id,int refnr,
					       const char* msg=0)	{}
};

#endif
