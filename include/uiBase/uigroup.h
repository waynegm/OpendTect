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

    mQtclass(QWidget)*	getWidget(int)  { return widget_; }

    void		detachWidgets();
    
    void		attach(uiBaseObject*,Relationship,uiBaseObject*);
    
    void		addChild(uiBaseObject*);
    void		removeChild(uiBaseObject*);
    
protected:

			uiGroup( const char* nm, mQtclass(QWidget)* );

    bool		circularRelationships() const;
    bool		updateLayout();
    
    bool		finalize();
    
    struct LayoutRelationship
    {
				LayoutRelationship(uiBaseObject*,uiBaseObject*,
						   uiBaseObject::Relationship);
	
	bool			relatesTo(const uiBaseObject* o) const;
	const uiBaseObject*	getOther(const uiBaseObject*) const;
	
	bool			operator==(const LayoutRelationship&) const;
	
	uiBaseObject*			obj0_;
	uiBaseObject*			obj1_;
	uiBaseObject::Relationship	relationship_;
    };
    
    bool		getLayout(int chld,RowCol& origin,RowCol& span) const;
    
    
    TypeSet<LayoutRelationship> relationships_;
    
    mQtclass(QGridLayout)*	gridlayout_;
    
    QWidget*			widget_;
    ObjectSet<uiBaseObject>	children_;
};


#endif
