#ifndef emhorizonutils_h
#define emhorizonutils_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "sets.h"
#include "ranges.h"

class RowCol;
class MultiID;
class od_ostream;
class BinIDValueSet;
class DataPointSet;
class HorSampling;
class BufferStringSet;
namespace Pos { class Provider; }
namespace PosInfo { class GeomID; }

namespace EM
{

class Surface;

/*!
\brief Group of utilities for horizons: here are all functions required in 
od_process_attrib_em for computing data on, along or between 2 horizons.
*/

mExpClass(EarthModel) HorizonUtils
{
public:
			HorizonUtils(){};
				
    static float 	getZ(const RowCol&,const Surface*);
    static float 	getMissingZ(const RowCol&,const Surface*,int);
    static Surface* 	getSurface(const MultiID&);
    static void 	getPositions(od_ostream&,const MultiID&,
				     ObjectSet<BinIDValueSet>&);
    static void 	getExactCoords(od_ostream&,const MultiID&,
	    			       const PosInfo::GeomID&,
				       const HorSampling&,
				       ObjectSet<DataPointSet>&);
    static void 	getWantedPositions(od_ostream&,ObjectSet<MultiID>&,
					   BinIDValueSet&,const HorSampling&,
					   const Interval<float>& extraz,
					   int nrinterpsamp,int mainhoridx,
					   float extrawidth,
					   Pos::Provider* provider=0);
    static void 	getWantedPos2D(od_ostream&,ObjectSet<MultiID>&,
				       DataPointSet*,const HorSampling&,
				       const Interval<float>& extraz,
				       const PosInfo::GeomID&);
    static bool		getZInterval(int idi,int idc,Surface*,Surface*,
	    			     float& topz,float& botz,int nrinterpsamp,
				     int mainhoridx,float& lastzinterval,
				     float extrawidth);
	
    static bool		SolveIntersect(float& topz,float& botz,int nrinterpsamp,
	    			       int is1main,float extrawidth,
				       bool is1interp,bool is2interp);
    static void 	addSurfaceData(const MultiID&,const BufferStringSet&,
				       const ObjectSet<BinIDValueSet>&);

protected:

};

};//namespace

#endif

