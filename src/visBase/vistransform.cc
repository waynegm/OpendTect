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

#include <osg/MatrixTransform>

mCreateFactoryEntry( visBase::Transformation );


namespace visBase
{

const char* Transformation::matrixstr()  { return "Matrix Row "; }

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


void Transformation::setRotation( const Coord3& vec, double angle )
{
    osg::Matrix osgmatrix = node_->getMatrix();
    const osg::Quat osgrotation( angle, osg::Vec3d(vec.x,vec.y,vec.z ) );
    osgmatrix.setRotate( osgrotation );
    node_->setMatrix( osgmatrix );
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
    return Coord3( vec.x(), vec.y(), vec.z() );
}


void Transformation::setScale( const Coord3& vec )
{
    osg::Matrix osgmatrix = node_->getMatrix();
    osgmatrix.makeScale( vec.x, vec.y, vec.z );
    node_->setMatrix( osgmatrix );
    updateNormalizationMode();
}


Coord3 Transformation::getScale() const
{
    const osg::Matrix matrix = node_->getMatrix();
    const osg::Vec3d vec = matrix.getScale();
    return Coord3( vec.x(), vec.y(), vec.z() );
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
    osg::Vec3d res( pos.x, pos.y, pos.z );
    transform( res );
    return Coord3( res[0], res[1], res[2] );
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
    osg::Vec3d res( pos.x, pos.y, pos.z );
    transformBack( res );
    return Coord3( res[0], res[1], res[2] );
}


void Transformation::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    const osg::Matrix matrix = node_->getMatrix();
    BufferString key = matrixstr(); key += 1; 
    par.set( key, matrix(0,0), matrix(1,0), matrix(2,0), matrix(3,0) );

    key = matrixstr(); key += 2;
    par.set( key, matrix(0,1), matrix(1,1), matrix(2,1), matrix(3,1) );

    key = matrixstr(); key += 3;
    par.set( key, matrix(0,2), matrix(1,2), matrix(2,2), matrix(3,2) );

    key = matrixstr(); key += 4;
    par.set( key, matrix(0,3), matrix(1,3), matrix(2,3), matrix(3,3) );
    return;
}


int Transformation::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    double matrix[4][4];
    BufferString key = matrixstr(); key += 1; 

    if ( !par.get( key, matrix[0][0],matrix[1][0],matrix[2][0],matrix[3][0] ))
	return -1;

    key = matrixstr(); key += 2;
    if ( !par.get( key, matrix[0][1],matrix[1][1],matrix[2][1],matrix[3][1] ))
	return -1;

    key = matrixstr(); key += 3;
    if ( !par.get( key, matrix[0][2],matrix[1][2],matrix[2][2],matrix[3][2] ))
	return -1;

    key = matrixstr(); key += 4;
    if ( !par.get( key, matrix[0][3],matrix[1][3],matrix[2][3],matrix[3][3] ))
	return -1;

    setA(   matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
	    matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
	    matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
	    matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3] );

    return 1;
}

		  

}; // namespace visBase
