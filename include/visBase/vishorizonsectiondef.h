#ifndef vishorizonsectiondef_h
#define vishorizonsectiondef_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________

-*/

// this header file is the set of const definitions used in the files related to
// horizonsection. don't include it in somewhere else in od.


static const int   cMaxNrTiles		= 15;
static const int   cMaxNrResolutions	= 10;

static const int   cNoTesselationNeeded	= 0;
static const int   cShouldRetesselate   = 1;
static const int   cMustRetesselate     = 2;

static const int   cNumberNodePerTileSide = 65;
static const int   cMaximumResolution = 6;
static const int   cMinInitialTimes = 3;
static const char  cNoneResolution = -1;

static const char* sKeySectionID() { return "Section ID"; }

enum	GeometryType{ Triangle = 0, Line,Point, WireFrame };


/* define the indexes of 9 tiles around this grid */
#   define LEFTUPTILE 0
#   define UPTILE 1
#   define RIGHTUPTILE 2
#   define LEFTTILE 3
#   define THISTILE 4
#   define RIGHTTILE 5
#   define LEFTBOTTOMTILE 6
#   define BOTTOMTILE 7
#   define RIGHTBOTTOMTILE 8

#define mDefineRCRange(section,symbol)\
    const StepInterval<int> rrg = section##symbol##userchangedisplayrg_ ?\
    section##symbol##displayrrg_ : section##symbol##geometry_->rowRange(); \
    const StepInterval<int>  crg = section##symbol##userchangedisplayrg_ ?\
    section##symbol##displaycrg_ : section##symbol##geometry_->colRange();\

#endif
