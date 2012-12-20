/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "vistransmgr.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "survinfo.h"
#include "arrayndimpl.h"
#include "cubesampling.h"
#include "linsolv.h"


namespace visSurvey
{

SceneTransformManager& STM()
{
    static SceneTransformManager* tm = 0;
    if ( !tm ) mTryAlloc( tm, SceneTransformManager );
    return *tm;
}
    

void SceneTransformManager::setZScale( mVisTrans* tf,
				       float zscale )
{
    if ( !tf ) return;

    const float zsc = zscale / 2;
    tf->setA(	1,	0,	0,	0,
	    	0,	1,	0,	0,
		0,	0,	zsc,	0,
		0,	0,	0,	1 );
}


mVisTrans* 
SceneTransformManager::createUTM2DisplayTransform( const HorSampling& hs ) const
{
    mVisTrans* tf = mVisTrans::create();

    const Coord startpos = SI().transform( hs.start );

    tf->setA(	1,	0,	0,	-startpos.x,
	    	0,	1,	0,	-startpos.y,
		0,	0,	-curzscale_,	0,
		0,	0,	0,	1 );
    return tf;
}


mVisTrans*
SceneTransformManager::createICRotationTransform( const HorSampling& hs ) const
{
    mVisTrans* tf = mVisTrans::create();
    computeICRotationTransform( *SI().get3DGeometry( true ), tf, 0 );
    return tf;
}


void SceneTransformManager::computeICRotationTransform( const InlCrlSystem& ics,
		visBase::Transformation* rotation,
		visBase::Transformation* disptrans ) const
{
    const HorSampling hs = ics.sampling().hrg;
    
    const BinID startbid = hs.start;
    const BinID stopbid = hs.stop;
    const BinID extrabid( startbid.inl, stopbid.crl );

    const Coord startpos = SI().transform( startbid );
    const Coord stoppos = SI().transform( stopbid );
    const Coord extrapos = SI().transform( extrabid );
    
    const float inldist = ics.inlDistance();
    const float crldist = ics.crlDistance();

    Array2DImpl<double> A(3,3);
    A.set( 0, 0, startbid.inl*inldist );
    A.set( 0, 1, startbid.crl*crldist );
    A.set( 0, 2, 1);

    A.set( 1, 0, stopbid.inl*inldist );
    A.set( 1, 1, stopbid.crl*crldist );
    A.set( 1, 2, 1);

    A.set( 2, 0, extrabid.inl*inldist );
    A.set( 2, 1, extrabid.crl*crldist );
    A.set( 2, 2, 1);

    double b[] = { 0, stoppos.x-startpos.x, extrapos.x-startpos.x };
    double x[3];

    LinSolver<double> linsolver( A );
    
    const int inlwidth = hs.inlRange().width();
    const int crlwidth = hs.crlRange().width();
    
    if ( !inlwidth )
    {
	if ( !crlwidth )
	{
	    rotation->reset();
	    return;
	}
	
	x[0] = 1;
	x[1] = b[1] / (crlwidth*crldist);
	x[2] = -startbid.inl*inldist - x[1] * startbid.crl*crldist;
    }
    else
	linsolver.apply( b, x );

    const double mat11 = x[0];
    const double mat12 = x[1];
    const double mat14 = x[2];

    b[0] = 0;
    b[1] = stoppos.y-startpos.y;
    b[2] = extrapos.y-startpos.y;

    if ( !crlwidth )
    {
	x[0] = b[1] / (inlwidth*inldist);
	x[1] = 1;
	x[2] = -x[0] * startbid.inl*inldist - startbid.crl*crldist;
    }
    else
    	linsolver.apply( b, x );

    const double mat21 = x[0];
    const double mat22 = x[1];
    const double mat24 = x[2];

    rotation->setA(	mat11,	mat12,	0,	mat14,
		mat21,	mat22,	0,	mat24,
		0,	0,	-1,	0,
		0,	0,	0,	1 );
    
    if ( disptrans )
	disptrans->setA( inldist,	0,		0,		0,
			 0,		crldist,	0,		0,
			 0,		0,		-curzscale_,	0,
			 0,		0,		0,		1 );
}

} // namespace visSurvey
