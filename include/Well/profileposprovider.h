#ifndef profileposprovider_h
#define profileposprovider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Feb 2017
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "coord.h"
#include "polylinend.h"

class ProfileModelBase;


mExpClass(Well) ProfilePosProvider
{
public:
    virtual			~ProfilePosProvider()		{}
    virtual Coord		getCoord(float pos) const	=0;
protected:
};


mExpClass(Well) ProfilePosProviderFromWellPos : public ProfilePosProvider
{
public:
				ProfilePosProviderFromWellPos(
					const ProfileModelBase& m )
				    : model_(m)
				    , sectionangle_(mUdf(float))
				    , sectionlength_(mUdf(float))	{}
    virtual			~ProfilePosProviderFromWellPos()	{}
    void			setAngle( float ang )	{ sectionangle_ = ang; }
    void			setLength( float len )	{ sectionlength_ = len;}
    virtual Coord		getCoord(float pos) const;
    static bool			isOK(const ProfileModelBase&);
protected:

    const ProfileModelBase&	model_;
    float			sectionangle_;
    float			sectionlength_;
};


mExpClass(Well) ProfilePosProviderFromLine : public ProfilePosProvider
{
public:
			ProfilePosProviderFromLine(const TypeSet<Coord>&);
    virtual		~ProfilePosProviderFromLine();
    virtual Coord	getCoord(float pos) const;
protected:

    const TypeSet<Coord>&	linepositions_;
    PolyLineND<Coord>*		linegeom_;
    double			arclength_;
};

#endif
