#ifndef nladataprep_h
#define nladataprep_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		May 2005
 RCS:		$Id: nladataprep.h,v 1.1 2005-05-12 14:08:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "nladesign.h"
class BinIDValueSet;
class PosVecDataSet;
template <class T> class Interval;

/*\brief Prepare data for usage in NLA training */

class NLADataPreparer
{
public:
    			NLADataPreparer( BinIDValueSet& bvs, int tc )
			    : bvs_(bvs), targetcol_(tc)		{}

    void		removeUndefs(bool targetonly=false);
    void		limitRange(const Interval<float>&);

    struct BalanceSetup
    {
			BalanceSetup()
			: nrclasses(0), nrptsperclss(0), noiselvl(0)	{}
	int		nrclasses, nrptsperclss;
	float		noiselvl;
    };
    void		balance(const BalanceSetup&);
    			//!< noiselvl not yet supported

protected:

    BinIDValueSet&	bvs_;
    int			targetcol_;

    void		addVecs(BinIDValueSet&,int,float);
    void		removeVecs(BinIDValueSet&,int);

};


#endif
