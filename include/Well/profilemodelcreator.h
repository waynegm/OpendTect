#ifndef profilemodelcreator_h
#define profilemodelcreator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "multiid.h"
#include "bufstring.h"

class BufferStringSet;
class ProfileModelBase;
class ProfilePosDataSet;
class TaskRunner;
class ZAxisTransform;
class ZValueProvider;
template <class T> class PolyLineND;



/*!\brief Base class for horizon users in profile mode */

mExpClass(Well) ProfileModelFromEventCreator
{
public:

    enum MovePol	{ MoveNone, MoveAll, MoveAbove, MoveBelow };

			ProfileModelFromEventCreator(ProfileModelBase&,
						     int maxnrpts=0);
    virtual		~ProfileModelFromEventCreator();
    virtual void	reset();

    bool		go(TaskRunner* tr=0);
    BufferString	errmsg_;

			// settable with default
    IOPar&		t2dpar_;
    int			maxnrprofs_;
    MovePol		movepol_; // only for Single, Multi ignores

protected:

    ProfileModelBase&		profs_;
    ProfilePosDataSet&	ppds_;
    ZAxisTransform*	t2dtr_;
    bool		needt2d_;

    void		addNewPositions();
    void		getKnownDepths(const ZValueProvider&,const char*);
    void		setMarkerDepths(const char*);

    virtual bool	doGo(TaskRunner*)		= 0;

    int			addNewProfilesAfter(int,int,bool);
    void		interpolateZOffsets();
    bool		doPush(float,float) const;
    bool		doPull(float,float) const;

};


/*!\brief uses horizon to set depths of control profiles. By default uses
  first well profile for T2D conversion. */

mExpClass(Well) ProfileModelFromSingleEventCreator
				: public ProfileModelFromEventCreator
{
public:

			ProfileModelFromSingleEventCreator(ProfileModelBase&);
    virtual void	reset();
    void		init();

    static bool		canDoWork(const ProfileModelBase&);

			// settable
    void		setZValProv( const ZValueProvider* zprov )
			{ zvalprov_ = zprov;}
    BufferString	marker_;
    float		sectionangle_;	// only used if model has exactly 1 well
    float		sectionlength_; // ditto

protected:

    const ZValueProvider* zvalprov_;

    virtual bool	doGo(TaskRunner*);
    void		fillCoords();

};


/*!\brief Uses multiple horizons in one go; does not use well coordinates but
 an external geometry.

 Important constraint: the horizon IDs must be sorted from top to bottom.

 */


mExpClass(Well) ProfileModelFromMultiEventCreator
				: public ProfileModelFromEventCreator
{
public:

				ProfileModelFromMultiEventCreator(
					ProfileModelBase&,
					const ObjectSet<ZValueProvider>&,
					const BufferStringSet& lvlnms,
					const TypeSet<Coord>& linegeom,
					int totalnrprofs=0);
				~ProfileModelFromMultiEventCreator();

protected:

    const ObjectSet<ZValueProvider>&	zvalprovs_;
    const BufferStringSet&		lvlnms_;

    virtual bool			doGo(TaskRunner*);
    void				fillCoords(const TypeSet<Coord>&);

};


#endif
