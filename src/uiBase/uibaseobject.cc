/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uibaseobject.h"

#include "uigroup.h"

uiBaseObject::uiBaseObject( const char* nm )
    : NamedObject(nm)
    , parent_( 0 )
{}


uiBaseObject::uiBaseObject( uiGroup* p, const char* nm )
    : NamedObject( nm )
    , parent_( 0 )
{
    if ( p ) p->addChild( this );
}


#define mMaxNrRows  32
#define mMaxNrCols  32

bool uiBaseObject::checkLayout() const
{
    bool array[mMaxNrRows][mMaxNrCols];
    memset( array, 0, mMaxNrCols*mMaxNrRows );
    
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
	const RowCol origin = getWidgetOrigin( idx );
	const RowCol span = getWidgetSpan( idx );
	
	for ( int idr=0; idr<span.row; idr++ )
	{
	    for ( int idc=0; idc<span.col; idc++ )
	    {
		const RowCol cell = origin + RowCol(idr,idc);
		if ( cell.row>=mMaxNrRows || cell.col>=mMaxNrCols )
		{
		    pErrMsg("Array overflow.");
		    return false;
		}
		
		if ( array[cell.row][cell.col] )
		{
		    pErrMsg("Widget overlap.");
		    return false;
		}
		
		array[cell.row][cell.col] = true;
	    }
	}
    }
    
    return true;
}


int uiBaseObject::getNrRows() const
{
    return getNrRowCols( true );
}


int uiBaseObject::getNrCols() const
{
    return getNrRowCols( false );
}


int uiBaseObject::getNrRowCols( bool row ) const
{
    int dim = row ? 0 : 1;
	
    int res = 0;
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
	const RowCol origin = getWidgetOrigin( idx );
	const RowCol span = getWidgetSpan( idx );
	
	const int nrrows = origin[dim]+span[dim];
	if ( nrrows>res )
	    res = nrrows;
    }
    
    return res;
}


void uiBaseObject::attach( Relationship rel, uiBaseObject* obj )
{
    if ( !parent() || !obj->parent() || obj->parent()!=parent() )
    {
	pErrMsg( "Invalid parents on attachment.");
	return;
    }
    
    parent()->attach( this, rel, obj );
}


const QWidget* uiBaseObject::getWidget(int idx) const
{ return const_cast<uiBaseObject*>(this)->getWidget( idx ); }


const uiGroup* uiBaseObject::parent() const
{ return const_cast<uiBaseObject*>(this)->parent(); }


void uiBaseObject::setParent( uiGroup* p )
{
    if ( parent_ && p )
	pErrMsg("Parent already set");
	
    parent_ = p;
}