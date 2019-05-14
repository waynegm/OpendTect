/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2019
________________________________________________________________________

-*/

#include "serverprogtool.h"
#include "ascstream.h"
#include "dbman.h"
#include "dbkey.h"
#include "genc.h"
#include "moddepmgr.h"
#include "odjson.h"
#include "odver.h"
#include "od_ostream.h"
#include <iostream>

static const char* sVersionCmd = "version";
static const char* sDontUseJSONCmd = "nojson";
static const char* sErrKey = "ERR";


static od_ostream& strm()
{
    return od_ostream::logStream();
}


ServerProgTool::ServerProgTool( int argc, char** argv, const char* moddep )
    : jsonroot_(*new JSONObject)
    , jsonmode_(true)
{
    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( moddep );
    clp_ = new CommandLineParser;
}


void ServerProgTool::initParsing( int protnr )
{
    protocolnr_ = protnr;

    if ( clp().nrArgs() < 1 )
	exitWithUsage();
    else if ( clp().hasKey( sVersionCmd ) )
    {
	od_cout() << GetFullODVersion() << " (" << protocolnr_ << ")"
		  << od_endl;
	exitProgram( true );
    }
    else if ( clp().hasKey(sDontUseJSONCmd) )
	jsonmode_ = false;

    setStatus( false ); // making sure this is the first entry

    uiRetVal uirv = DBM().setDataSource( clp() );
    if ( !uirv.isOK() )
	respondError( toString(uirv) );
}


ServerProgTool::~ServerProgTool()
{
    exitProgram( true );
}


template <class T>
void ServerProgTool::setSingle( const char* keyw, T val, JSONObject* jobj )
{
    if ( !jsonmode_ )
	iop_.set( keyw, val );
    else
    {
	if ( !jobj )
	    jobj = &jsonroot_;

	BufferString cleankeyw( keyw ); cleankeyw.clean();
	jobj->set( cleankeyw, val );
    }
}


template <class T>
void ServerProgTool::setArr( const char* keyw, const T* vals, size_type sz,
			     JSONObject* jobj )
{
    if ( !jsonmode_ )
    {
	const TypeSet<T> valset( vals, sz );
	iop_.set( keyw, valset );
    }
    else
    {
	if ( !jobj )
	    jobj = &jsonroot_;

	BufferString cleankeyw( keyw ); cleankeyw.clean();
	auto* arr = jobj->set( cleankeyw, new JSONArray(false) );
	arr->set( vals, sz );
    }
}


template <class T>
void ServerProgTool::setArr( const char* keyw, const T& valset,
			     JSONObject* jobj )
{
    if ( !jsonmode_ )
	iop_.set( keyw, valset );
    else
    {
	if ( !jobj )
	    jobj = &jsonroot_;

	BufferString cleankeyw( keyw ); cleankeyw.clean();
	auto* arr = jobj->set( cleankeyw, new JSONArray(false) );
	arr->set( valset );
    }
}


#define mDefServerProgToolSetSingleFn( typ, arg, val ) \
    void ServerProgTool::set( const char* keyw, typ arg, JSONObject* jobj ) \
    { setSingle( keyw, val, jobj ); }

mDefServerProgToolSetSingleFn( const char*, str, str )
mDefServerProgToolSetSingleFn( const DBKey&, dbky, dbky.toString() )
mDefServerProgToolSetSingleFn( bool, val, val )
mDefServerProgToolSetSingleFn( od_int16, val, val )
mDefServerProgToolSetSingleFn( od_uint16, val, val )
mDefServerProgToolSetSingleFn( od_int32, val, val )
mDefServerProgToolSetSingleFn( od_uint32, val, val )
mDefServerProgToolSetSingleFn( od_int64, val, val )
mDefServerProgToolSetSingleFn( float, val, val )
mDefServerProgToolSetSingleFn( double, val, val )



#define mDefServerProgToolSetSetFn( typ ) \
    void ServerProgTool::set( const char* keyw, const typ& val, \
			      JSONObject* jobj ) \
    { setArr( keyw, val, jobj ); }

#define mDefServerProgToolSetArrFn( typ ) \
    void ServerProgTool::set( const char* keyw, const typ* val, size_type sz, \
			      JSONObject* jobj ) \
    { setArr( keyw, val, sz, jobj ); }

#define mDefServerProgToolSetFns( typ ) \
    mDefServerProgToolSetSetFn( TypeSet<typ> ) \
    mDefServerProgToolSetArrFn( typ )


mDefServerProgToolSetSetFn( BufferStringSet )
mDefServerProgToolSetSetFn( DBKeySet )
mDefServerProgToolSetSetFn( BoolTypeSet )
mDefServerProgToolSetFns( od_int16 )
mDefServerProgToolSetFns( od_uint16 )
mDefServerProgToolSetFns( od_int32 )
mDefServerProgToolSetFns( od_uint32 )
mDefServerProgToolSetFns( od_int64 )
mDefServerProgToolSetFns( float )
mDefServerProgToolSetFns( double )


void ServerProgTool::set( const char* keyw, const bool* vals, size_type sz,
			  JSONObject* jobj )
{
    if ( !jsonmode_ )
    {
	BoolTypeSet bset;
	for ( auto idx=0; idx<sz; idx++ )
	    bset.add( vals[idx] );
	iop_.set( keyw, bset );
    }
    else
    {
	if ( !jobj )
	    jobj = &jsonroot_;

	BufferString cleankeyw( keyw ); cleankeyw.clean();
	auto* arr = jobj->set( cleankeyw, new JSONArray(false) );
	arr->set( vals, sz );
    }
}


void ServerProgTool::set( const char* keyw, JSONObject* jobj )
{
    jsonroot_.set( keyw, jobj );
}


void ServerProgTool::set( const char* keyw, JSONArray* jarr )
{
    jsonroot_.set( keyw, jarr );
}


void ServerProgTool::setStatus( bool success )
{
    set( "Status", success ? "OK" : "Fail" );
}


void ServerProgTool::respondInfo( bool success, bool exit )
{
    setStatus( success );
    if ( jsonmode_ )
    {
	od_ostream strm( std::cout );
	jsonroot_.write( strm );
    }
    else
    {
	ascostream ascstrm( strm() );
	iop_.putTo( ascstrm );
    }

    if ( exit )
	exitProgram( success );
}


void ServerProgTool::respondError( const char* msg )
{
    set( sErrKey, msg );
    respondInfo( false );
}


void ServerProgTool::addToUsageStr( BufferString& str,
			const char* flg, const char* args, bool isextra )
{
    const bool isfirst = str.isEmpty();
    str.add( "\n  " );
    if ( isextra )
	str.add( "[--" );
    else
	str.add( isfirst ? "   --" : "|| --" );
    str.add( flg );
    if ( args && *args )
	str.add( " " ).add( args );
    if ( isextra )
	str.add( "]" );
}


void ServerProgTool::exitWithUsage()
{
    BufferString msg( "Usage: ", GetExecutableName() );
    msg.add( getSpecificUsage() );
    addToUsageStr( msg, CommandLineParser::sDataRootArg(), "data_root_dir",
		    true );
    addToUsageStr( msg, CommandLineParser::sSurveyArg(), "survey_dir", true );
    addToUsageStr( msg, sDontUseJSONCmd, "", true );
    od_cout() << msg << od_endl;
    exitProgram( false );
}


void ServerProgTool::exitProgram( bool success )
{
    ExitProgram( success ? 0 : 1 );
}
