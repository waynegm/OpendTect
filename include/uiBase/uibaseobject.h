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
#include "rowcol.h"

mFDQtclass(QWidget);
class uiGroup;

mClass(uiBase) uiBaseObject : public NamedObject
{
public:
    enum Relationship	{ Below, Above, Left, Right, RowAligned, ColumnAligned,
	AlignedBelow, AlignedAbove, AlignedLeft, AlignedRight };
    
    void			attach(Relationship,uiBaseObject*);
    
    virtual int			getHAlignRow() const		{ return 0; }
    virtual int			getVAlignCol() const		{ return 0; }
    
    virtual int			getNrWidgets() const	{ return 1; }
    virtual mQtclass(QWidget)*	getWidget(int)		{ return 0; }
    const mQtclass(QWidget)*	getWidget(int) const;
    virtual RowCol		getWidgetOrigin(int) const {return RowCol(0,0);}
    virtual RowCol		getWidgetSpan(int) const   {return RowCol(1,1);}

    int				getNrRows() const;
    int				getNrCols() const;
    
    virtual uiGroup*		parent() { return parent_; }
    const uiGroup*		parent() const;
    
    virtual void		detachWidgets() {}
    /*!< Widget is now dead. Don't touch it any more */
    
protected:
				friend class uiGroup;
    virtual void		setParent(uiGroup*);
    virtual bool		checkLayout() const;
    int				getNrRowCols(bool row) const;
    
    virtual bool		finalize()			{ return true; }
    
    virtual bool		updateLayout()			{ return true; }
				uiBaseObject(const char* nm);
				uiBaseObject(uiGroup* parent, const char* nm);
    
    ObjectSet<uiBaseObject>	attachedsiblings_;
    uiGroup*			parent_;
};


#define mQStringToConstChar( str )		str.toAscii().constData()


#endif
