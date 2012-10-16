/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visnodestate.h"

#include <osg/StateSet>

using namespace visBase;


NodeState::~NodeState()
{
    setStateSet( 0 );
}


void NodeState::setStateSet( osg::StateSet* ns )
{
    if ( stateset_ )
    {
	for ( int idx=0; idx<attributes_.size(); idx++ )
	    stateset_->removeAttribute( attributes_[idx] );
    }
    
    stateset_ = ns;
    
    if ( stateset_ )
    {
	for ( int idx=0; idx<attributes_.size(); idx++ )
	    stateset_->setAttribute( attributes_[idx] );
    }
}


void NodeState::doAdd( osg::StateAttribute* as)
{
    if ( stateset_ )
	stateset_->setAttribute( as );
    
    attributes_ += as;
}


void NodeState::doRemove( osg::StateAttribute* as)
{
    if ( stateset_ )
	stateset_->removeAttribute( as );
    
    attributes_ -= as;
}
