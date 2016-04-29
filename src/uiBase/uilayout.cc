/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          18/08/1999
________________________________________________________________________

-*/


#include "uilayout.h"

#include "uiobj.h"

#include <QGridLayout>


uiConstraint::uiConstraint( constraintType tp, i_LayoutItem* o, int marg )
    : other_(o)
    , type_(tp)
    , margin_(marg)
    , enabled_(true)
{
    if ( !other_ && (type_<leftBorder || type_>hCentered) )
    { pErrMsg("No attachment defined!!"); }
}


bool uiConstraint::operator==( const uiConstraint& oth ) const
{
    return type_ == oth.type_ && other_ == oth.other_
	&& margin_ == oth.margin_ && enabled_ == oth.enabled_;
}


bool uiConstraint::operator!=( const uiConstraint& oth ) const
{ return !(*this == oth); }

bool uiConstraint::enabled() const		{ return enabled_ ; }
void uiConstraint::disable( bool yn=true )	{ enabled_ = !yn; }


uiLayoutMgr::uiLayoutMgr( uiGroup* g )
    : layout_( 0 )
    , hcenterobj_( 0 )
    , halignobj_( 0 )
    , group_( g )
{}


uiLayoutMgr::~uiLayoutMgr()
{
    detachAllNotifiers();
}


void uiLayoutMgr::addObject( uiObject* itm )
{
    if ( !itm ) return;

    mAttachCB( itm->objectToBeDeleted(), uiLayoutMgr::objectDeletedCB );
    objects_.addIfNew( itm );
}


bool uiLayoutMgr::isAdded( const uiObject* obj ) const
{
    return objects_.isPresent( obj );
}


void uiLayoutMgr::enableOwnGrid()
{
    if ( !layout_ ) layout_ = new QGridLayout;
}


bool uiLayoutMgr::attach(const uiObject* a, constraintType rel,
                         const uiObject* b)
{
    const int prevrelsize = relationships_.size();
    if ( rel==alignedAbove || rel==alignedBelow )
    {
        relationships_ += Relationship( a, b, hAligned );
        rel = rel==alignedBelow ? belowOf : aboveOf;
    }
    
    relationships_ += Relationship( a, b, rel );
    
    if ( hasCircularRelationships() )
    {
        while ( relationships_.size()>prevrelsize )
            relationships_.removeSingle( prevrelsize );
        
        pErrMsg("Circular relationship detected.");
        return false;
    }
    
    return true;
}


bool uiLayoutMgr::hasCircularRelationships() const
{ return false; }


void uiLayoutMgr::setHCenterObj( const uiObject* obj )
{
    if ( !isAdded( obj ) )
    {
        pErrMsg( "Cannot align on object that is not present");
        return;
    }
    
    if ( hcenterobj_ && hcenterobj_!=obj )
    {
        pErrMsg( "HCenter already set");
        return;
    }
}


void uiLayoutMgr::setHAlignObj( const uiObject* obj )
{
    if ( !isAdded( obj ) )
    {
        pErrMsg( "Cannot align on object that is not present");
        return;
    }
    
    if ( halignobj_ && halignobj_!=obj )
    {
        pErrMsg( "HCenter already set");
        return;
    }
}


void uiLayoutMgr::populateGrid()
{    
    if ( !layout_ )
        return;
    
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
        RowCol origin(0,0), span(1,1);
        if ( !computeLayout( idx, origin, span ) )
            pErrMsg( "Layout calculation failed");
        
        for ( int idy=0; idy<objects_[idx]->getNrWidgets(); idy++ )
        {
            const RowCol widgetorigin = objects_[idx]->getWidgetOrigin( idy );
            const RowCol widgetspan = objects_[idx]->getWidgetSpan( idy );
        
            layout_->addWidget( objects_[idx]->getWidget(idy),
                            origin.row()+widgetorigin.row(),
                            origin.col()+widgetorigin.col(),
                            widgetspan.row(), widgetspan.col() );
        }
    }
}


bool uiLayoutMgr::computeLayout( const uiObject* obj, RowCol& origin,
                                RowCol& span ) const
{
    const int idx = objects_.indexOf( obj );
    return idx>=0
        ? computeLayout( idx, origin, span )
        : false;
}


bool uiLayoutMgr::computeLayout( int objectidx, RowCol& origin,
                                 RowCol& span ) const
{
    const uiObject* obj = objects_.validIdx( objectidx )
        ? objects_[objectidx]
        : 0;
    
    if ( !obj )
        return false;
    
    TypeSet<Relationship> relationships;
    for ( int idx=0; idx<relationships_.size(); idx++ )
    {
        if ( relationships_[idx].obj0_ == obj )
            relationships += relationships_[idx];
    }
    
    origin = RowCol(0,0);
    span = obj->layoutSpan();
    
    if ( !relationships.isEmpty() )
    {
        bool foundrowalignment = false;
        bool foundcolalignment = false;
        
        for ( int idx=0; idx<relationships.size(); idx++ )
        {
            const uiObject* sibling = relationships[idx].getOther( obj );
            const int siblingidx = objects_.indexOf( sibling );
            RowCol siblingorigin, siblingspan;
            if ( !computeLayout( siblingidx, siblingorigin, siblingspan ))
                return false;

            switch ( relationships[idx].type_ )
            {
                case aboveOf:
                    origin.row() = siblingorigin.row()-span.row();
                    break;
                case belowOf:
                    origin.row() = siblingorigin.row()+siblingspan.row();
                    break;
                case leftOf:
                    origin.col() = siblingorigin.col()-span.col();
                    break;
                case rightOf:
                    origin.col() = siblingorigin.col()+siblingspan.col();
                    break;
                case vAligned:
                    if ( foundrowalignment )
                    {
                        pErrMsg( "Only single row alignment allowed.");
                        return false;
                    }
                    foundrowalignment = true;
                    origin.row() = siblingorigin.row();
                    break;
                case hAligned:
                    if ( foundcolalignment )
                    {
                        pErrMsg( "Only single col alignment allowed.");
                        return false;
                    }
                    foundcolalignment = true;
                    origin.col() = siblingorigin.col();
                    break;
                default:
                    pErrMsg("Unhandled relationship");
            }
            
        }
    }
    
    return true;
}


void uiLayoutMgr::objectDeletedCB(CallBacker* o)
{
    uiObject* obj = (uiObject*) o;
    objects_ -= obj;
    
    for ( int idx=relationships_.size()-1; idx>=0; idx-- )
    {
        if ( relationships_[idx].contains( obj ) )
            relationships_.removeSingle( idx );
    }
}


bool uiLayoutMgr::Relationship::operator==(const uiLayoutMgr::Relationship& b) const
{
    return obj0_==b.obj0_ && obj1_==b.obj1_ && type_==b.type_;
}


