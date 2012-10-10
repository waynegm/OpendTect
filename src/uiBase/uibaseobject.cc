/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/08/1999
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uibaseobject.h"

#include "uigroup.h"

uiBaseObject::uiBaseObject( const char* nm )
    : NamedObject(nm)
{}


void uiBaseObject::attach( uiLayout::Relationship rel, uiBaseObject* obj )
{
    if ( !parent() || !obj->parent() || obj->parent()!=parent() )
    {
	pErrMsg( "Invalid parents on attachment.");
	return;
    }
    
    parent()->attach( this, rel, obj );
}


const QWidget* uiBaseObject::getWidget(int r, int c) const
{ return const_cast<uiBaseObject*>(this)->getWidget( r, c ); }


const uiGroup* uiBaseObject::parent() const
{ return const_cast<uiBaseObject*>(this)->parent(); }

