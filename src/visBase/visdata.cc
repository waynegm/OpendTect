/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visdata.h"

#include "errh.h"
#include "keystrs.h"
#include "visdataman.h"
#include "visselman.h"

#include <osg/Node>
#include <osg/ValueObject>
#include <osgDB/WriteFile>


mExternC(visBase) void refOsgObj( void* obj )
{
    ((osg::Referenced*) obj)->ref();
}


mExternC(visBase) void unrefOsgObj( void* obj )
{
    ((osg::Referenced*) obj)->unref();
}


namespace visBase
{

void DataObject::enableTraversal( unsigned int tt, bool yn )
{
    if ( osgNode() )
    {
	unsigned int mask = osgNode()->getNodeMask();
	osgNode()->setNodeMask( yn ? (mask | tt) : (mask & ~tt) );
    }
}


bool DataObject::isTraversalEnabled( unsigned int tt ) const
{
    return osgNode() && (osgNode()->getNodeMask() & tt);
}


const char* DataObject::name() const
{
    osg::ref_ptr<const osg::Node> osgnode = osgNode();
    if ( osgnode ) return osgnode->getName().c_str();
    
    return !name_ || name_->isEmpty() ? 0 : name_->buf();
}


void DataObject::setName( const char* nm )
{
    osg::ref_ptr<osg::Node> osgnode = osgNode();
    
    if ( osgnode )
	osgnode->setName( nm );
    else if ( nm )
    {
	if ( !name_ ) name_ = new BufferString;
    }
    
    if ( name_ ) (*name_) = nm;
}


DataObject::DataObject()
    : id_( -1 )
    , name_( 0 )
{
    DM().addObject( this );
}


DataObject::~DataObject()
{
    DM().removeObject( this );
    delete name_;
}


void DataObject::setID( int nid )
{
    id_ = nid;
    updateOsgNodeData();
}


void DataObject::updateOsgNodeData()
{
    osg::Node* osgnode = gtOsgNode();
    if ( !osgnode )
	return;

    osgnode->setName( name_ ? name_->buf() : sKey::EmptyString().str() );

    static std::string idstr( sKey::ID() );
    osgnode->setUserValue( idstr, id() );
}


void DataObject::select() const
{ DM().selMan().select( id() ); }


void DataObject::deSelect() const
{ DM().selMan().deSelect( id() ); }


bool DataObject::isSelected() const
{ return selectable() && DM().selMan().selected().indexOf( id()) != -1; }


void DataObject::setDisplayTransformation( const mVisTrans* trans )
{   
    if ( trans!=getDisplayTransformation() )
	pErrMsg("Not implemented");
}   
    

bool DataObject::serialize( const char* filename, bool binary )
{
    if ( !osgNode() )
	return true;
    
    return osgDB::writeNodeFile( *osgNode(), std::string( filename ) );
}


}; // namespace visBase
