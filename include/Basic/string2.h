#ifndef string2_h
#define string2_h

/*@+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		11-4-1994
 Contents:	Extra string functions
 RCS:		$Id: string2.h,v 1.17 2006-07-25 13:02:52 cvsbert Exp $
________________________________________________________________________
-*/



#include "gendefs.h"
#include "plftypes.h"

#ifdef __cpp__
extern "C" {
#endif


/*!> bluntly puts a '\0' on trailing white space. */
void		removeTrailingBlanks(char*);
/*!> advances given pointer to first non-whitespace. */
#define skipLeadingBlanks(ptr) \
    { while ( (ptr) && *(ptr) && isspace(*(ptr)) ) (ptr)++; }

/*!> stricmp with option to compare part */
int		caseInsensitiveEqual(const char*,const char*,
				     int nr_chars_to_match_0_is_all);
/*!> checks whether a string is the start of another string. */
int		matchString(const char* startstring,const char* maybebigger);
/*!> is a case insensitive version of matchString */
int		matchStringCI(const char*,const char*);

/*!> fills a string with another padded with blanks (to the right). */
void		getPaddedString(const char*,char*,int max_nr_from_inp_0_is_all);

/*!> fills a buffer with the next word (delimited by whitespace) in string.
     It returns a ptr just after the word. */
const char*	getNextWord(const char*,char*);
/*!> returns the value for a value token `XXX=xxx'. */
int		getEqTokenValue(const char*,const char*,float*);

/*!> counts occurrences of a char in string */
int		countCharacter(const char*,char);
/*!> replaces all occurrences of a char with another */
void		replaceCharacter(char*,char,char);
/*!> replaces all occurrences of a string with another */
void		replaceString(char*,const char*,const char*);
/*!> removes all occurrences of a char */
void		removeCharacter(char*,char);
/*!> cleans a string from non-alpha numeric by replacing with underscores. */
void		cleanupString(char*,int,int,int);
/*!> tells whether a string holds a parseable number */
int		isNumberString(const char*,int int_only);

/*!> returns the string for an int in a static buffer. You'll have to pass a
     C-style ('printf') format. */
const char*	getStringFromInt(const char* fmt,int32);
const char*	getStringFromUInt(const char* fmt,uint32);
/*!> is like getStringFromInt. Treats "%lg" special: it will do removal of
     trailing zeros and use %f in more cases than std. */
const char*	getStringFromInt64(const char* fmt,int64);
const char*	getStringFromUInt64(const char* fmt,uint64);
const char*	getStringFromDouble(const char* fmt,double);
/*!> is like getStringFromDouble, with special %lf treatment. */
const char*	getStringFromFloat(const char* fmt,float);
/*!> removes unwanted zeros and dots from a floating point in string. */
void		prettyNumber(char*,int is_float);

/*!> returns ptr to static buffer with "yes" or "No" */
const char*	getYesNoString(int);
/*!> returns 1 or 0 by inspecting string */
int		yesNoFromString(const char*);
/*!> returns "th" or "st" or "nd" or "rd"; like in 1st, 2nd, 3rd etc. */
const char*	getRankPostFix(int);


#ifdef __cpp__
}
#include <stdlib.h>
#include "undefval.h"

inline const char* toString( double d )	{ return getStringFromDouble(0, d ); }
inline const char* toString( int32 i )	{ return getStringFromInt(0, i ); }
inline const char* toString( uint32 i )	{ return getStringFromUInt(0, i ); }
inline const char* toString( int64 i )	{ return getStringFromInt64(0, i ); }
inline const char* toString( uint64 i )	{ return getStringFromUInt64(0, i ); }
inline const char* toString( float f )	{ return getStringFromFloat(0, f ); }
inline const char* toString( bool b )	{ return getYesNoString( b ); }

#define mImplGetFromStrFunc( type, func, udfv ) \
inline bool getFromString( type& i, const char* s, type undef=udfv ) \
{ \
    if ( s ) \
    { \
	char* e; \
	i = (type)func; \
	if ( e==s ) \
	{ \
	    i = undef; \
	    return false;\
	}\
	return true; \
    } \
 \
    i = undef; \
    return false; \
}



// inline bool getFromString( double& d, const char* s, double udefval );
mImplGetFromStrFunc(double, strtod(s,&e), mUdf(double) )
// inline bool getFromString( float& d, const char* s, float udefval );
mImplGetFromStrFunc(float, strtod(s,&e), mUdf(float) )
// inline bool getFromString( int& d, const char* s, int udefval );
mImplGetFromStrFunc(int, strtol(s,&e,10), mUdf(int) )
#undef mImplGetFromStrFunc

inline bool getFromString( bool& b, const char* s )
{
    if ( s )
    {
	b = ( yesNoFromString( s ) ? true : false );
	return true;
    }

    b = false;
    return false;
}


#endif


#endif
