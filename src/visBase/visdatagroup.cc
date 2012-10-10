/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visdatagroup.h"
#include "visdataman.h"
#include "iopar.h"

#include <Inventor/nodes/SoSeparator.h>

#include <osg/Group>

mCreateFactoryEntry( visBase::DataObjectGroup );

namespace visBase
{

const char* DataObjectGroup::nokidsstr()	{ return "Number of Children"; }
const char* DataObjectGroup::kidprefix()	{ return "Child "; }

DataObjectGroup::DataObjectGroup()
    : osggroup_( new osg::Group )
    , separate_( true )
    , change( this )
    , righthandsystem_( true )
{
    osggroup_->ref();
}


DataObjectGroup::~DataObjectGroup()
{
    mObjectSetApplyToAll( objects_, objects_[idx]->setParent( 0 ));
    
    deepUnRef( objects_ );

    osggroup_->unref();
}


int DataObjectGroup::size() const
{ return objects_.size(); }


#define mNewObjectOperations \
no->ref(); \
no->setRightHandSystem( isRightHandSystem() ); \
no->setParent( this ); \
change.trigger()

void DataObjectGroup::addObject( DataObject* no )
{
    objects_ += no;
    
    if ( osggroup_ && no->osgNode() ) osggroup_->addChild( no->osgNode() );

    mNewObjectOperations;
}


void DataObjectGroup::setDisplayTransformation( const mVisTrans* nt )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->setDisplayTransformation(nt);
}


const mVisTrans* DataObjectGroup::getDisplayTransformation() const
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	if ( objects_[idx]->getDisplayTransformation() )
	    return objects_[idx]->getDisplayTransformation();

    return 0;
}


void DataObjectGroup::setRightHandSystem( bool yn )
{
    righthandsystem_ = yn;

    yn = isRightHandSystem();

    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->setRightHandSystem( yn );
}


bool DataObjectGroup::isRightHandSystem() const
{ return righthandsystem_; }


void DataObjectGroup::addObject(int nid)
{
    DataObject* no =
	dynamic_cast<DataObject*>( DM().getObject(nid) );

    if ( !no ) return;
    addObject( no );
}


void DataObjectGroup::insertObject( int insertpos, DataObject* no )
{
    if ( insertpos>=size() ) return addObject( no );

    objects_.insertAt( no, insertpos );
    
    if ( no->osgNode() ) osggroup_->insertChild( insertpos, no->osgNode() );
    mNewObjectOperations;
}


int DataObjectGroup::getFirstIdx( int nid ) const
{
    const DataObject* sceneobj =
	(const DataObject*) DM().getObject(nid);

    if ( !sceneobj ) return -1;

    return getFirstIdx( sceneobj );
}


int DataObjectGroup::getFirstIdx( const DataObject* sceneobj ) const
{ return objects_.indexOf(sceneobj); }


void DataObjectGroup::removeObject( int idx )
{
    DataObject* sceneobject = objects_[idx];
    osggroup_->removeChild( sceneobject->osgNode() );

    objects_.remove( idx );

    sceneobject->setParent( 0 );
    sceneobject->unRef();
    change.trigger();
}


void DataObjectGroup::removeAll()
{
    while ( size() ) removeObject( 0 );
}


osg::Node* DataObjectGroup::gtOsgNode()
{
    return osggroup_;
}


void DataObjectGroup::fillPar( IOPar& par, TypeSet<int>& saveids)const
{
    DataObject::fillPar( par, saveids );
    
    par.set( nokidsstr(), objects_.size() );
    
    BufferString key;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	key = kidprefix();
	key += idx;

	int saveid = objects_[idx]->id();
	if ( saveids.indexOf( saveid )==-1 ) saveids += saveid;

	par.set( key, saveid );
    }
}


int DataObjectGroup::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    int nrkids;
    if ( !par.get( nokidsstr(), nrkids ) )
	return -1;

    BufferString key;
    TypeSet<int> ids;
    for ( int idx=0; idx<nrkids; idx++ )
    {
	key = kidprefix();
	key += idx;

	int newid;
	if ( !par.get( key, newid ) )
	    return -1;

	if ( !DM().getObject( newid ) )
	{
	    res = 0;
	    continue;
	}

	ids += newid;
    }

    for ( int idx=0; idx<ids.size(); idx++ )
	addObject( ids[idx] );

    return res;
}

}; // namespace visBase
