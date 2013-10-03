#ifndef matlabstep_h
#define matlabstep_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "multiinputmod.h"
#include "volprocchain.h"

namespace VolProc
{

mExpClass(MultiInput) MultiInputStep : public Step
{
public:
			mDefaultFactoryInstantiation( Step,
				MultiInputStep,
				"MultiInputStep",
				"MultiInput" );

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    bool		needsInput() const 		{ return true; }
    int			getNrInputs() const;
    bool		canInputAndOutputBeSame() const	{ return true; }
    bool		needsFullVolume() const		{ return true; }
    bool		areSamplesIndependent() const	{ return false; }
    const char*		errMsg() const		{ return errmsg_.str(); }

    Task*		createTask();

protected:

			MultiInputStep();
			~MultiInputStep();

    FixedString		errmsg_;
};

} // namespace VolProc

#endif

