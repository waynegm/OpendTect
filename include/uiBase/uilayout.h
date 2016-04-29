#ifndef uilayout_h
#define uilayout_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          03/03/2000
________________________________________________________________________

-*/
#include "uibasemod.h"
#include "gendefs.h"
#include "callback.h"
#include "rowcol.h"
#include "manobjectset.h"

mFDQtclass(QLayout);
mFDQtclass(QGridLayout);
mFDQtclass(QWidget);


class i_LayoutItem;
class uiObject;
class uiGroup;

enum constraintType 
{ 
    leftOf, rightOf, //!< LeftOf/RightOf atach widgets tightly together
    leftTo, rightTo, //!< LeftTo/RightTo allow extra horizonal distance
    leftAlignedBelow, leftAlignedAbove,
    rightAlignedBelow, rightAlignedAbove,
    alignedWith, alignedBelow, alignedAbove,	//!< Uses uiObject::horAlign()
    centeredBelow, centeredAbove,	//!< Uses i_LayoutItem::centre()
    centeredLeftOf, centeredRightOf,	//!< Uses i_LayoutItem::centre()
    ensureLeftOf, ensureRightOf,
    ensureBelow,
    leftBorder, rightBorder, topBorder, bottomBorder,
    hCentered,				//!< Centers with respect to parent
    heightSameAs, widthSameAs,
    stretchedBelow, stretchedAbove,   //!< stretches widget to horiz. borders 
    stretchedLeftTo, stretchedRightTo, //!< stretches widget to vertical borders
    atSamePosition,
    hAligned, vAligned, aboveOf, belowOf
};


mExpClass(uiBase) uiConstraint
{
friend class i_LayoutItem;
public:
			uiConstraint(constraintType,i_LayoutItem* o,int marg);

    bool		operator==(const uiConstraint&) const;
    bool		operator!=(const uiConstraint&) const;

    bool		enabled() const;
    void		disable(bool yn);

protected:
    constraintType      type_;
    i_LayoutItem*       other_;
    int                 margin_;
    bool		enabled_;
};


mExpClass(uiBase) uiLayoutMgr : public CallBacker
{
public:
			uiLayoutMgr(uiGroup*);
			~uiLayoutMgr();
    void                addObject(uiObject*);
    bool                attach(const uiObject*,constraintType,const uiObject*);
    bool		isAdded(const uiObject*) const;
    
    void                setHCenterObj(const uiObject*);
    const uiObject*	getHCenterObj() const { return hcenterobj_; }
    void                setHAlignObj(const uiObject*);
    const uiObject*	getHAlignObj() const { return halignobj_; }
    
    void		enableOwnGrid();
    bool		hasOwnGrid() const { return qLayout(); }
    
    void		populateGrid();

    mQtclass(QLayout)*          qLayout();
    const mQtclass(QLayout)*    qLayout() const;

    bool                        computeLayout(const uiObject*,RowCol& orig,
                                              RowCol& span ) const;
private:
    
    void                        objectDeletedCB(CallBacker*);
    bool                        hasCircularRelationships() const;
    bool                        computeLayout(int objectidx,RowCol& origin,
                                              RowCol& span ) const;

    
    struct Relationship
    {
                                Relationship(const uiObject* a,const uiObject* b,
                                             constraintType rel)
                                    : obj0_( a ), obj1_( b ), type_( rel ) {}
        
        const uiObject*     	getOther(const uiObject* a) const
                                { return a==obj0_ ? obj1_ : obj0_; }
        bool                    contains(const uiObject* o) const
                                { return o==obj0_ || o==obj1_; }
        
        bool                    operator==(const Relationship&) const;
        
        const uiObject*         obj0_;
        const uiObject*         obj1_;
        constraintType          type_;
    };
    
    bool			layoutonparent_;
    TypeSet<Relationship>       relationships_;
    ObjectSet<uiObject>     	objects_;
    const uiObject*         	hcenterobj_;
    const uiObject*         	halignobj_;
    uiGroup*                    group_;
    
    mQtclass(QGridLayout)*      layout_;
};

#endif
