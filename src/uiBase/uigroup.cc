/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uigroup.h"

#include "QWidget"

#define mStandarInstantiations \
relationships_( false ) \
, parent_( 0 ) \
, gridlayout_( 0 )

uiGroup::uiGroup( const char* nm )
    : uiBaseObject( nm )
    , widget_( new QWidget )
    , mStandarInstantiations
{}


uiGroup::uiGroup( const char* nm, mQtclass(QWidget)* w )
    : uiBaseObject( nm )
    , widget_( w )
    , mStandarInstantiations
{
    
}


uiGroup::~uiGroup()
{
    if ( parent_ )
	pErrMsg("Not removed from parent yet!");
    
    delete widget_;
    mObjectSetApplyToAll( children_, children_[idx]->setParent( 0 ) );
    deepErase( children_ );
}


void uiGroup::detachWidgets()
{
    inherited::detachWidgets();
    widget_ = 0;
}


void uiGroup::attach(uiBaseObject* a,uiLayout::Relationship rel,uiBaseObject* b)
{
    relationships_ += new LayoutRelationship( a, b, rel );
    if ( circularRelationships() )
    {
	relationships_.pop();
	pErrMsg("Circular relationship detected.");
    }
}


void uiGroup::addChild( uiBaseObject* child )
{
    children_ += child;
    child->setParent( this );
}


void uiGroup::removeChild( uiBaseObject* child )
{
    const int idx = children_.indexOf( child );
    
    child->setParent( 0 );
    delete children_.remove( idx );
}


void uiGroup::setParent(uiGroup* p)
{
    if ( parent_ )
	pErrMsg("Parent already set");
    
    parent_ = p;
}

bool uiGroup::circularRelationships() const
{
    return false;
}


bool uiGroup::updateLayout()
{
    return true;
}


uiGroup::LayoutRelationship::LayoutRelationship(uiBaseObject* a,uiBaseObject* b,
		       uiLayout::Relationship rel)
    : obj0_( a )
    , obj1_( b )
    , relationship_( rel )
{}
