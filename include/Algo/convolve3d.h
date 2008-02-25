#ifndef convolve3d_h
#define convolve3d_h

/*@+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Feb 2008
 RCS:           $Id: convolve3d.h,v 1.3 2008-02-25 19:05:58 cvskris Exp $
________________________________________________________________________


*/

#include "arraynd.h"
#include "task.h"
#include "rowcol.h"

/*!Convolves (or correlates) two 3D signals. */


template <class T>
class Convolver3D : public ParallelTask
{
public:
    inline		Convolver3D();

    inline void		setX(const Array3D<T>&,
	    		     int first0=0,int first1=0, int first2=0);
    inline void		setY(const Array3D<T>&,
	    		     int first0=0,int first1=0, int first2=0);
    inline void		setZ(Array3D<T>& z )		{ z_ = &z; }
    void		setNormalize( bool n )		{ normalize_ = n; }
    			/*!<If true, the sum will be divided with the number
			    of valid samples. */
    void		setCorrelate( bool yn )		{ correlate_ = yn; }
    			/*!<If true, the convolution will be replaced by a
			   correllation. */

protected:
    inline bool		doWork( int, int, int );
    int			totalNr() const { return z_->info().getTotalSz(); }
    const Array3D<T>*	x_;
    int			xshift0_;
    int			xshift1_;
    int			xshift2_;
    const Array3D<T>*	y_;
    int			yshift0_;
    int			yshift1_;
    int			yshift2_;

    Array3D<T>*		z_;
    bool		normalize_;
    bool		correlate_;
};


template <class T> inline
Convolver3D<T>::Convolver3D()
    : x_( 0 )
    , xshift0_( 0 )
    , xshift1_( 0 )
    , xshift2_( 0 )
    , y_( 0 )
    , yshift0_( 0 )
    , yshift1_( 0 )
    , yshift2_( 0 )
    , z_( 0 )
    , normalize_( false )
    , correlate_( false )
{}


template <class T> inline
void Convolver3D<T>::setX( const Array3D<T>& x,
			   int first0, int first1, int first2 )
{
    x_ = &x;
    xshift0_ = first0;
    xshift1_ = first1;
    xshift2_ = first2;
}


template <class T> inline
void Convolver3D<T>::setY( const Array3D<T>& y, int first0, int first1,
       			   int first2 )
{
    y_ = &y;
    yshift0_ = first0;
    yshift1_ = first1;
    yshift2_ = first2;
}


template <class T> inline
bool Convolver3D<T>::doWork( int start, int stop, int )
{
    const int xsz0 = x_->info().getSize( 0 );
    const int xsz1 = x_->info().getSize( 1 );
    const int xsz2 = x_->info().getSize( 2 );
    const int ysz0 = y_->info().getSize( 0 );
    const int ysz1 = y_->info().getSize( 1 );
    const int ysz2 = y_->info().getSize( 2 );

    int startpos[3];

    if ( !z_->info().getArrayPos( start, startpos ) )
	return false;

    ArrayNDIter iterator( z_->info() );
    iterator.setPos( startpos );

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int* zvar = iterator.getPos();
	T sum = 0;
	int nrsamples = 0;
	for ( int idx0=0; idx0<xsz0; idx0++ )
	{
	    const int xvar0 = idx0-xshift0_;
	    const int yvar0 = correlate_ ? xvar0-zvar[0] : zvar[0]-xvar0;
	    const int idy0 = yvar0+yshift0_;
	    if ( idy0<0 || idy0>=ysz0 )
		continue;

	    for ( int idx1=0; idx1<xsz1; idx1++ )
	    {
		const int xvar1 = idx1-xshift1_;
		const int yvar1 = correlate_ ? xvar1-zvar[1] : zvar[1]-xvar1;
		const int idy1 = yvar1+yshift1_;
		if ( idy1<0 || idy1>=ysz1 )
		    continue;

		for ( int idx2=0; idx2<xsz2; idx2++ )
		{
		    const int xvar2 = idx2-xshift2_;
		    const int yvar2 = correlate_
			? xvar2-zvar[2]
			: zvar[2]-xvar2;
		    const int idy2 = yvar2+yshift2_;
		    if ( idy2<0 || idy2>=ysz2 )
			continue;

		    const T yval = y_->get( idy0, idy1, idy2 );
		    if ( mIsUdf(yval) )
			continue;

		    const T xval = x_->get( idx0, idx1, idx2 );
		    if ( mIsUdf(xval) )
			continue;

		    sum += xval * yval;
		    nrsamples++;
		}
	    }
	}

	if ( !nrsamples ) z_->set( zvar, 0 );
	else if ( normalize_ ) z_->set( zvar, sum/nrsamples );
	else z_->set( zvar, sum );

	if ( !iterator.next() && idx!=stop )
	    return false;
    }

    return true;
}

#endif
