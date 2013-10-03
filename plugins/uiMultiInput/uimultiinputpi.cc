/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : February 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uimultiinputmod.h"

#include "uimultiinputstep.h"
#include "odplugin.h"


mDefODPluginInfo(uiMultiInput)
{
    static PluginInfo retpi = {
	"MultiInput test plugin",
	"dGB Earth Sciences",
	"=od",
    	"Test plugin for multiple volume builder inputs."
	    "\nThis is the User interface of the test plugin." };
    return &retpi;
}


mDefODInitPlugin(uiMultiInput)
{
    VolProc::uiMultiInputStep::initClass();

    // Add custom dir
    return 0;
}
