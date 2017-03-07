#ifndef profileviewpars_h
#define profileviewpars_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "stratlevel.h"

class IOPar;

mExpClass(Well) ProfileViewPars
{
public:
			ProfileViewPars()
			    : skipz_(0)
			    , vertstack_(3)
			    , drawwells_(true)
			    , drawwellnames_(true)
			    , drawctrls_(true)
			    , drawmarkers_(true)
			    , drawmarkernames_(true)
			    , drawctrlprofmrkrnms_(false)
			    , drawconnections_(true)
			{ Strat::LVLS().getNames( selmrkrnms_ ); }
    bool		operator ==(const ProfileViewPars&) const;

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    float		skipz_;
    int			vertstack_; // multiplier for modeling block size
    bool		drawwells_;
    bool		drawwellnames_;
    bool		drawctrls_;
    bool		drawmarkers_;
    bool		drawconnections_;
    bool		drawmarkernames_;
    bool		drawctrlprofmrkrnms_;
    BufferStringSet	selmrkrnms_;

};

#endif
