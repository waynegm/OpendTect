/*+
_______________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR:	Nanne Hemstra
 DATE:		February 2013
_______________________________________________________________________________

 -*/
static const char* rcsID mUsedVar = "$Id$";

#include "multiinputstep.h"

#include "arraynd.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "task.h"


namespace VolProc
{

class MultiInputTask : public ParallelTask
{
public:
MultiInputTask( MultiInputStep& step, const Array3D<float>& in1,
	        const Array3D<float>& in2, Array3D<float>& out )
    : input1_(in1)
    , input2_(in2)
    , output_(out)
    , step_(step)
{
    totalnr_ = in1.info().getTotalSz();
}

    bool	init();
    od_int64	nrIterations() const	{ return totalnr_; }
    bool	doWork(od_int64,od_int64,int);
    const char*	message() const		{ return message_.buf(); }

protected:

    const Array3D<float>&	input1_;
    const Array3D<float>&	input2_;
    Array3D<float>&		output_;
    MultiInputStep&		step_;
    BufferString		message_;
    od_int64			totalnr_;
};


bool MultiInputTask::init()
{
    return true;
}


bool MultiInputTask::doWork( od_int64 start, od_int64 stop, int )
{
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	const int arridx = mCast(int,idx);
	const float val1 = input1_.getData()[arridx];
	const float val2 = input2_.getData()[arridx];
	output_.getData()[arridx] = val1 - val2;
    }

    return true;
}


// MultiInputStep
MultiInputStep::MultiInputStep()
{}

MultiInputStep::~MultiInputStep()
{}


Task* MultiInputStep::createTask()
{
    const Attrib::DataCubes* input1 = getInput( getInputSlotID(0) );
    const Attrib::DataCubes* input2 = getInput( getInputSlotID(1) );
    Attrib::DataCubes* output = getOutput( getOutputSlotID(0) );
    if ( !input1 || input1->nrCubes()<1 )
    {
	errmsg_ = "No first input provided.";
	return 0;
    }

    if ( !input2 || input2->nrCubes()<1 )
    {
	errmsg_ = "No second input provided.";
	return 0;
    }

    if ( !output || output->nrCubes()<1 )
    {
	errmsg_ = "No output provided.";
	return 0;
    }

    MultiInputTask* task =
	new MultiInputTask( *this, input1->getCube(0), input2->getCube(0),
			    output->getCube(0) );
    if ( !task->init() )
    {
	errmsg_ = task->message();
	delete task;
	task = 0;
    }

    return task;
}


int MultiInputStep::getNrInputs() const
{ return 2; }


void MultiInputStep::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
}


bool MultiInputStep::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    return true;
}

} // namespace VolProc
