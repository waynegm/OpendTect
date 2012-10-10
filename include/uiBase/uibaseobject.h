#ifndef uibaseobject2_h
#define uibaseobject2_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "namedobj.h"
#include "geometry.h"

class QWidget;
class uiParent;
class uiGroup;

mStruct(uiBase) uiLayout
{
    enum Relationship	{ AlignedBelow, AligneAbove };
};


mClass(uiBase) uiBaseObject : public NamedObject
{
public:
    
    void			attach(uiLayout::Relationship,uiBaseObject*);
    
    virtual int			getHAlignRow() const		{ return 0; }
    virtual int			getVAlignCol() const		{ return 0; }
    virtual int			getNrWidgetRows() const		{ return 1; }
    virtual int			getNrWidgetCols() const		{ return 1; }
    virtual int			getRowSpan(int r, int c) const	{ return 1; }
    virtual int			getColSpan(int r, int c) const	{ return 1; }
    virtual mQtclass(QWidget)*	getWidget(int r, int c)		{ return 0; }
    const mQtclass(QWidget)*	getWidget(int r, int c) const;
    
    virtual uiGroup*		parent() { return 0; }
    const uiGroup*		parent() const;
    
    virtual void		detachWidgets() {}
    /*!< Widget is now dead. Don't touch it any more */
    
protected:
				friend class uiGroup;
    virtual void		setParent(uiGroup*)		{}
    
    virtual bool		updateLayout()			{ return true; }
				uiBaseObject(const char* nm);
    
    ObjectSet<uiBaseObject>	attachedsiblings_;
};


#endif
