/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visobject.h"

#include "errh.h"
#include "iopar.h"
#include "visdataman.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vistransform.h"

#include <osg/Switch>
#include <osg/Material>

namespace visBase
{

const char* VisualObjectImpl::sKeyMaterialID()	{ return "Material ID"; }
const char* VisualObjectImpl::sKeyIsOn()	{ return "Is on"; }


VisualObject::VisualObject( bool issel )
    : isselectable(issel)
    , deselnotifier(this)
    , selnotifier(this)
    , rightClick(this)
    , rcevinfo(0)
{}


VisualObject::~VisualObject()
{}


VisualObjectImpl::VisualObjectImpl( bool issel )
    : VisualObject( issel )
    , osgroot_( new osg::Switch )
    , material_( 0 )
    , righthandsystem_( true )
{
}


VisualObjectImpl::~VisualObjectImpl()
{
    if ( material_ ) material_->unRef();
}


void VisualObjectImpl::setLockable()
{
}


void VisualObjectImpl::readLock()
{
}
	

void VisualObjectImpl::readUnLock()
{
}

bool VisualObjectImpl::tryReadLock()
{
    return false;
}


void VisualObjectImpl::writeLock()
{
}
	

void VisualObjectImpl::writeUnLock()
{
}


bool VisualObjectImpl::tryWriteLock()
{
    return false;
}



void VisualObjectImpl::turnOn( bool yn )
{

    if ( yn )
	osgroot_->setAllChildrenOn();
    else
	osgroot_->setAllChildrenOff();
}


bool VisualObjectImpl::isOn() const
{
    if ( osgroot_->getNumChildren() )
    {
	return osgroot_->getValue( 0 );
    }
    
    return true;
}


void VisualObjectImpl::setMaterial( Material* nm )
{
    if ( material_ )
    {
	osgroot_->getOrCreateStateSet()->removeAttribute(
						material_->getMaterial() );
	material_->unRef();
    }

    material_ = nm;

    if ( material_ )
    {
	material_->ref();
	osgroot_->getOrCreateStateSet()->setAttribute(material_->getMaterial());
	osgroot_->getStateSet()->setDataVariance( osg::Object::DYNAMIC );
    }
}
    
    
Material* VisualObjectImpl::getMaterial()
{
    if ( !material_ )
	setMaterial( visBase::Material::create() );
    
    return material_;
}


void VisualObjectImpl::removeSwitch()
{
    pErrMsg( "Don't call");
}


osg::Node* VisualObjectImpl::gtOsgNode()
{
    return osgroot_;
}


void VisualObjectImpl::addChild( SoNode* nn )
{  }


void VisualObjectImpl::insertChild( int pos, SoNode* nn )
{  }


void VisualObjectImpl::removeChild( SoNode* nn )
{  }


int VisualObjectImpl::childIndex( const SoNode* nn ) const
{ return -1;}


int VisualObjectImpl::addChild( osg::Node* nn )
{
    if ( !nn )
	return -1;
    
    return osgroot_->addChild( nn );
}


void VisualObjectImpl::insertChild( int pos, osg::Node* nn )
{
    osgroot_->insertChild( pos, nn );
}


void VisualObjectImpl::removeChild( osg::Node* nn )
{
    osgroot_->removeChild( nn );
}


int VisualObjectImpl::childIndex( const osg::Node* nn ) const
{
    const int idx = osgroot_->getChildIndex(nn);
    if ( idx==osgroot_->getNumChildren() )
	return -1;
    return idx;
}


SoNode* VisualObjectImpl::getChild(int idx)
{ return 0; }


int VisualObjectImpl::usePar( const IOPar& iopar )
{
    int res = VisualObject::usePar( iopar );
    if ( res != 1 ) return res;

    int matid;
    if ( iopar.get(sKeyMaterialID(),matid) )
    {
	if ( matid==-1 ) setMaterial( 0 );
	else
	{
	    DataObject* mat = DM().getObject( matid );
	    if ( !mat ) return 0;
	    if ( typeid(*mat) != typeid(Material) ) return -1;

	    setMaterial( (Material*)mat );
	}
    }
    else
	setMaterial( 0 );

    bool isonsw;
    if ( iopar.getYN(sKeyIsOn(),isonsw) )
	turnOn( isonsw );

    return 1;
}


void VisualObjectImpl::fillPar( IOPar& iopar,
					 TypeSet<int>& saveids ) const
{
    VisualObject::fillPar( iopar, saveids );
    iopar.set( sKeyMaterialID(), material_ ? material_->id() : -1 );

    if ( material_ && saveids.indexOf(material_->id()) == -1 )
	saveids += material_->id();

    iopar.setYN( sKeyIsOn(), isOn() );
}


void VisualObject::triggerRightClick( const EventInfo* eventinfo )
{
    rcevinfo = eventinfo;
    rightClick.trigger();
}


bool VisualObject::getBoundingBox( Coord3& minpos, Coord3& maxpos ) const
{
    pErrMsg( "Not impl. Not sure if needed." );
    return false;
    /*
    SbViewportRegion vp;
    SoGetBoundingBoxAction action( vp );
    action.apply( const_cast<SoNode*>(getInventorNode()) );
    const SbBox3f bbox = action.getBoundingBox();

    if ( bbox.isEmpty() )
	return false;

    const SbVec3f min = bbox.getMin();
    const SbVec3f max = bbox.getMax();

    minpos.x = min[0]; minpos.y = min[1]; minpos.z = min[2];
    maxpos.x = max[0]; maxpos.y = max[1]; maxpos.z = max[2];

    const Transformation* trans =
	const_cast<VisualObject*>(this)->getDisplayTransformation();
    if ( trans )
    {
	minpos = trans->transformBack( minpos );
	maxpos = trans->transformBack( maxpos );
    }

    return true;
     */
}


const TypeSet<int>* VisualObject::rightClickedPath() const
{
    return rcevinfo ? &rcevinfo->pickedobjids : 0;
}

}; //namespace
