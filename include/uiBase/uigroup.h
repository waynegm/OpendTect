#ifndef uigroup2_h
#define uigroup2_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uibaseobject.h"
#include "manobjectset.h"

mFDQtclass(QGridLayout);

mClass(uiBase) uiGroup : public uiBaseObject
{ typedef uiBaseObject inherited;
public:
			uiGroup( const char* nm="uiGroup" );
    virtual		~uiGroup();

    uiGroup*		parent()	    { return parent_; }
    mQtclass(QWidget)*	getWidget(int,int)  { return widget_; }

    void		detachWidgets();
    
    void		attach(uiBaseObject*,uiLayout::Relationship,
			       uiBaseObject*);
    
    void		addChild(uiBaseObject*);
    void		removeChild(uiBaseObject*);
    
protected:

			uiGroup( const char* nm, mQtclass(QWidget)* );
    void		setParent(uiGroup*);
    
    bool		circularRelationships() const;
    bool		updateLayout();
    
    
    struct LayoutRelationship
    {
	LayoutRelationship(uiBaseObject*,uiBaseObject*,
			   uiLayout::Relationship);
	
	uiBaseObject*		obj0_;
	uiBaseObject*		obj1_;
	uiLayout::Relationship	relationship_;
    };
    
    
    ManagedObjectSet<LayoutRelationship>    relationships_;
    
    mQtclass(QGridLayout)*		    gridlayout_;
    
    uiGroup*				    parent_;
    QWidget*				    widget_;
    ObjectSet<uiBaseObject>		    children_;
};


#endif
