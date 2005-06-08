#ifndef datacoldef_h
#define datacoldef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jan 2005
 RCS:		$Id: datacoldef.h,v 1.3 2005-06-08 16:45:34 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
class UnitOfMeasure;


/*!\brief Column definition in tabular data sets

  The ref_ is intended for whatever references that are important in the
  software but which should probably not be displayed to the user.

  All but the 'user name' is optional.

*/

class DataColDef
{
public:
				DataColDef( const char* nm, const char* ref=0,
					    const UnitOfMeasure* un=0 )
				: name_(nm), ref_(ref), unit_(un)	{}

    BufferString		name_;
    BufferString		ref_;
    const UnitOfMeasure*	unit_;

    enum MatchLevel		{ Exact, Start, None };
    MatchLevel			compare(const DataColDef&,bool use_name) const;
				//!< if !use_name, matches ref_ .

    void			putTo(BufferString&) const;
    void			getFrom(const char*);

    static const DataColDef&	unknown();

};


#endif
