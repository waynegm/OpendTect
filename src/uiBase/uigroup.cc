/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uigroup.h"

#include "rowcol.h"

#include <QWidget>
#include <QGridLayout>

#define mStandarInstantiations \
relationships_( false ) \
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


void uiGroup::attach(uiBaseObject* a,Relationship rel,uiBaseObject* b)
{
    const int prevrelsize = relationships_.size();
    if ( rel==AlignedBelow || rel==AlignedAbove )
    {
	relationships_ += new LayoutRelationship( a, b, ColumnAligned );
	rel = rel==AlignedBelow ? Below : Above;
    }
    relationships_ += new LayoutRelationship( a, b, rel );
    if ( circularRelationships() )
    {
	while ( relationships_.size()>prevrelsize )
	    delete relationships_.pop();
	
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
    delete children_.removeSingle( idx );
}


bool uiGroup::circularRelationships() const
{
    return false;
}


bool uiGroup::updateLayout()
{
    return true;
}


bool uiGroup::finalize()
{
    if ( gridlayout_ )
	return true;
    
    TypeSet<RowCol> origins;
        
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( !children_[idx]->finalize() )
	    return false;
	
	RowCol origin, span;
	if ( !getLayout( idx, origin, span ) )
	    return false;
	
	origins += origin;
    }
    
    gridlayout_ = new QGridLayout( widget_ );
    
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	const RowCol& origin = origins[idx];
	for ( int idy=0; idy<children_[idx]->getNrWidgets(); idy++ )
	{
	    QWidget* childwidget = children_[idx]->getWidget( idy );
	    
	    RowCol widgetorigin = origin+children_[idx]->getWidgetOrigin(idy);
	    RowCol widgetspan = children_[idx]->getWidgetSpan( idy );
	    
	    
	    gridlayout_->addWidget( childwidget,
				       widgetorigin.row, widgetorigin.col,
				       widgetspan.row, widgetspan.col );
	}
    }
    
    return  true;
}


bool uiGroup::getLayout( int chld, RowCol& origin,
			RowCol& span ) const
{
    const uiBaseObject* obj = children_.validIdx( chld )
        ? children_[chld]
	: 0;
    
    if ( !obj )
	return false;
    
    ObjectSet<const LayoutRelationship> relationships;
    for ( int idx=0; idx<relationships_.size(); idx++ )
    {
	if ( relationships_[idx]->obj0_ == obj )
	    relationships += relationships_[idx];
    }
    
    origin = RowCol(0,0);
    span = RowCol( obj->getNrRows(), obj->getNrCols() );
    
    if ( !relationships.isEmpty() )
    {
	bool foundrowalignment = false;
	bool foundcolalignment = false;
	
	for ( int idx=0; idx<relationships.size(); idx++ )
	{
	    const uiBaseObject* sibling = relationships[idx]->getOther( obj );
	    const int siblingidx = children_.indexOf( sibling );
	    RowCol siblingorigin, siblingspan;
	    if ( !getLayout( siblingidx, siblingorigin, siblingspan ))
		return false;
	    
	    switch ( relationships[idx]->relationship_ )
	    {
		case uiBaseObject::Above:
		    origin.row = siblingorigin.row-span.row;
		    break;
		case uiBaseObject::Below:
		    origin.row = siblingorigin.row+siblingspan.row;
		    break;
		case uiBaseObject::Left:
		    origin.col = siblingorigin.col-span.col;
		    break;
		case uiBaseObject::Right:
		    origin.col = siblingorigin.col+siblingspan.col;
		    break;
		case uiBaseObject::RowAligned:
		    if ( foundrowalignment )
		    {
			pErrMsg( "Only single row alignment allowed.");
			return false;
		    }
		    foundrowalignment = true;
		    origin.row = siblingorigin.row;
		    break;
		case uiBaseObject::ColumnAligned:
		    if ( foundcolalignment )
		    {
			pErrMsg( "Only single col alignment allowed.");
			return false;
		    }
		    foundcolalignment = true;
		    origin.col = siblingorigin.col;
		    break;
		default:
		    pErrMsg("Unhandled relationship");
	    }
	    
	}
    }
    
    return true;
}


uiGroup::LayoutRelationship::LayoutRelationship(uiBaseObject* a,uiBaseObject* b,
		       uiBaseObject::Relationship rel)
    : obj0_( a )
    , obj1_( b )
    , relationship_( rel )
{}


bool uiGroup::LayoutRelationship::relatesTo( const uiBaseObject* obj ) const
{
    return getOther( obj );
}


const uiBaseObject*
uiGroup::LayoutRelationship::getOther( const uiBaseObject* obj ) const
{
    if ( obj==obj0_ )
	return obj1_;
    if ( obj==obj1_ )
	return obj0_;
    
    return 0;
}
