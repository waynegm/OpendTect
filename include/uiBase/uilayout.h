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
class uiBaseObject;
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
    void                addObject(uiBaseObject*);
    bool                attach(uiBaseObject*,constraintType,uiBaseObject*);
    bool		isAdded(const uiBaseObject*) const;
    
    void                setHCenterObj(const uiBaseObject*);
    void                setHAlignObj(const uiBaseObject*);
    
    void		enableOwnGrid();
    bool		hasOwnGrid() const { return qLayout(); }
    
    void		populateGrid();

    mQtclass(QLayout)*          qLayout();
    const mQtclass(QLayout)*    qLayout() const;

    bool                        computeLayout(const uiBaseObject*,RowCol& orig,
                                              RowCol& span ) const;
private:
    
    void                        objectDeletedCB(CallBacker*);
    bool                        hasCircularRelationships() const;
    bool                        computeLayout(int objectidx,RowCol& origin,
                                              RowCol& span ) const;

    
    struct Relationship
    {
                                Relationship(uiBaseObject* a,uiBaseObject* b,
                                             constraintType rel)
                                    : obj0_( a ), obj1_( b ), type_( rel ) {}
        
        const uiBaseObject*     getOther(const uiBaseObject* a) const
                                { return a==obj0_ ? obj1_ : obj0_; }
        bool                    contains(const uiBaseObject* o) const
                                { return o==obj0_ || o==obj1_; }
        
        bool                    operator==(const Relationship&) const;
        
        uiBaseObject*           obj0_;
        uiBaseObject*           obj1_;
        constraintType          type_;
    };
    
    bool			layoutonparent_;
    TypeSet<Relationship>       relationships_;
    ObjectSet<uiBaseObject>     objects_;
    const uiBaseObject*         hcenterobj_;
    const uiBaseObject*         halignobj_;
    uiGroup*                    group_;
    
    mQtclass(QGridLayout)*      layout_;
};

#endif
