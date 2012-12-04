/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vistransform.h"
#include "iopar.h"
#include "trigonometry.h"
#include "visosg.h"

#include <osg/MatrixTransform>

mCreateFactoryEntry( visBase::Transformation );


namespace visBase
{

Transformation::Transformation()
    : node_( 0 )
{
    if ( osggroup_ ) osggroup_->unref();
    
    osggroup_ = node_ = new osg::MatrixTransform;
    osggroup_->ref();
}


Transformation::~Transformation()
{
    //node is unreffed in visBase::DataObjectGroup
}

    
#define mUpdateOsgMatrix( statement ) \
    osg::Vec3d trans, scale; \
    osg::Quat rot, so; \
    node_->getMatrix().decompose( trans, rot, scale, so ); \
    statement; \
    osg::Matrix mat = osg::Matrix::inverse( osg::Matrix::rotate(so) ); \
    mat *= osg::Matrix::scale( scale ); \
    mat *= osg::Matrix::rotate( so ); \
    mat *= osg::Matrix::rotate( rot ); \
    mat *= osg::Matrix::translate( trans ); \
    node_->setMatrix( mat );


void Transformation::setRotation( const Coord3& vec, double angle )
{
    mUpdateOsgMatrix( rot = osg::Quat(angle,Conv::to<osg::Vec3d>(vec)) );
}


void Transformation::setTranslation( const Coord3& vec )
{
    osg::Matrix osgmatrix = node_->getMatrix();
    osgmatrix.setTrans( vec.x, vec.y, vec.z );
    node_->setMatrix( osgmatrix );
}


Coord3 Transformation::getTranslation() const
{
    const osg::Matrix matrix = node_->getMatrix();
    const osg::Vec3d vec = matrix.getTrans();
    return Conv::to<Coord3>( vec );
}


void Transformation::setScale( const Coord3& vec )
{
    mUpdateOsgMatrix( scale = Conv::to<osg::Vec3d>(vec) );
    updateNormalizationMode();
}


Coord3 Transformation::getScale() const
{
    const osg::Matrix matrix = node_->getMatrix();
    const osg::Vec3d vec = matrix.getScale();
    return Conv::to<Coord3>( vec );
}


void Transformation::setTransRotScale( const Coord3& trans,                   
				       const Coord3& rot,double angle,  
				       const Coord3& scale ) 
{
    osg::Matrix mat = osg::Matrix::scale( Conv::to<osg::Vec3d>(scale) ); 
    mat *= osg::Matrix::rotate( osg::Quat(angle,Conv::to<osg::Vec3d>(rot)) );
    mat *= osg::Matrix::translate( Conv::to<osg::Vec3d>(trans) );
    node_->setMatrix( mat );
    updateNormalizationMode();
}


void Transformation::setAbsoluteReferenceFrame()
{
    node_->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
}


void Transformation::reset()
{
    node_->setMatrix( osg::Matrix::identity() );
    return;
}


void Transformation::setA( double a11, double a12, double a13, double a14,
			   double a21, double a22, double a23, double a24,
			   double a31, double a32, double a33, double a34,
			   double a41, double a42, double a43, double a44 )
{
    node_->setMatrix( osg::Matrix(
			a11, a21, a31, a41,
			a12, a22, a32, a42,
			a13, a23, a33, a43,
			a14, a24, a34, a44 ) );

    updateNormalizationMode();
}


void Transformation::updateNormalizationMode()
{
    const double eps = 1e-5;
    const osg::Vec3d scale = node_->getMatrix().getScale();

    if ( node_->getStateSet() )
    {
	node_->getStateSet()->removeMode( GL_NORMALIZE );
	node_->getStateSet()->removeMode( GL_RESCALE_NORMAL );
    }

    if ( fabs(scale.x()-scale.y()) > eps ||
	 fabs(scale.y()-scale.z()) > eps ||
	 fabs(scale.z()-scale.x()) > eps )
    {
	node_->getOrCreateStateSet()->setMode( GL_NORMALIZE,
					       osg::StateAttribute::ON );
    }
    else if ( fabs(scale.x()-1.0) > eps ||
	      fabs(scale.y()-1.0) > eps ||
	      fabs(scale.z()-1.0) > eps )
    {
	node_->getOrCreateStateSet()->setMode( GL_RESCALE_NORMAL,
					       osg::StateAttribute::ON );
    }
}


Coord3 Transformation::transform( const Coord3& pos ) const
{
    osg::Vec3d res( Conv::to<osg::Vec3d>( pos ) );
    transform( res );
    return Conv::to<Coord3>( res );
}
    
    
void Transformation::transform( const Coord3& pos, osg::Vec3f& resf ) const
{
    osg::Vec3d res( Conv::to<osg::Vec3d>( pos ) );
    transform( res );
    
    resf[0] = (float) res[0];
    resf[1] = (float) res[1];
    resf[2] = (float) res[2];
}


void Transformation::transform( osg::Vec3d& res ) const
{
//TODO Check that we should use preMult
    res = node_->getMatrix().preMult( res );
} 


void Transformation::transformBack( osg::Vec3d& res ) const
{
//TODO Check that we should use preMult, the inverse or postMult
    res = osg::Matrixd::inverse(node_->getMatrix()).preMult( res );
}


Coord3 Transformation::transformBack( const Coord3& pos ) const
{
    osg::Vec3d res = Conv::to<osg::Vec3d>( pos );
    transformBack( res );
    return Conv::to<Coord3>( res );
}


}; // namespace visBase
