#ifndef profileview2model_h
#define profileview2model_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "ranges.h"


/* Transforms between model positions and view positions (and vv)

Model [0 - 1] maps to View [1 - NrSeq]. But displayed area is [0.5 - NrSeq+0.5].
Therefore clipping delivers a number in the range [0 - 1] for model X,
but [0.5 - NrSeq+0.5] for view X.
*/

mExpClass(Well) ProfileView2Model
{
public:
			ProfileView2Model(int nrseq=1);

    int			nrSeq() const		{ return nrseq_; }
    Interval<float>	zRange() const			{ return zrg_; }
    void		setNrSeq(int);
    void		setXRange( Interval<float> r )	{ xrg_ = r; }
    void		setZRange( Interval<float> r )	{ zrg_ = r; }
    void		setZInDepth(bool zindepth)	{ zindepth_ = zindepth;}
    bool		zInDepth() const		{ return zindepth_; }

    Interval<float>	modelXRange() const	{ return Interval<float>(0,1); }
    Interval<float>	viewXRange() const;
    Interval<float>	modelZRange() const	{ return zrg_; }
    Interval<float>	viewZRange() const;

    float		modelX(float viewx,bool clipped=false) const;
    float		viewX(float modelx,bool clipped=false) const;
    float		modelZ(float viewz) const;
    float		viewZ(float modelz) const;

protected:

    int			nrseq_;
    Interval<float>	xrg_;
    Interval<float>	zrg_;
    const bool		zinft_;
    bool		zindepth_;

};

#endif
