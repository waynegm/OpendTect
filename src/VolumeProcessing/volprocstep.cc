/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : October 2006
-*/


#include "volprocstep.h"

#include "arrayndimpl.h"
#include "seisdatapack.h"
#include "posinfodetector.h"
#include "volprocchain.h"
#include "keystrs.h"


mImplClassFactory( VolProc::Step, factory );


namespace VolProc
{

class BinIDWiseTask : public ParallelTask
{ mODTextTranslationClass(BinIDWiseTask);
public:

		BinIDWiseTask( Step& ro )
		    : step_( ro ), totalnr_( -1 )
		{
		    setName(ro.userName());
		    msg_ = tr("Executing Volume Builder Task");
		}

    uiString	message() const	{ return msg_; }
    uiString	nrDoneText() const	{ return ParallelTask::sTrcFinished(); }

protected:

    bool	doWork(od_int64 start, od_int64 stop, int threadid )
		{
		    const TrcKeySampling hrg(
				step_.output_->sampling().hsamp_ );
		    BinID curbid = hrg.start_;

		    const int nrinls = mCast( int, start/hrg.nrCrl() );
		    const int nrcrls = mCast( int, start - nrinls*hrg.nrCrl() );
		    curbid.inl() += nrinls*hrg.step_.inl();
		    curbid.crl() += nrcrls*hrg.step_.crl();

		    for ( int idx=mCast(int,start); idx<=stop; idx++ )
		    {
			if ( !step_.computeBinID( curbid, threadid ) )
			    return false;

			addToNrDone( 1 );

			if ( idx>=stop )
			    break;

			if ( !shouldContinue() )
			    return false;

			curbid.crl() += hrg.step_.crl();
			if ( curbid.crl()>hrg.stop_.crl() )
			{
			    curbid.crl() = hrg.start_.crl();
			    curbid.inl() += hrg.step_.inl();
			    if ( curbid.inl()>hrg.stop_.inl() )
			    {
				pErrMsg("Going outside range");
				return false;
			    }
			}
		    }

		    return true;
		}

    od_int64	nrIterations() const
		{
		    if ( totalnr_==-1 )
		    {
			const TrcKeySampling hrg(
				step_.output_->sampling().hsamp_ );
			totalnr_ = hrg.nrInl() * hrg.nrCrl();
		    }

		    return totalnr_;
		}

    bool	doPrepare( int nrthreads )
		{
		    const bool res = step_.prepareWork( nrthreads );
		    if ( !res ) msg_ = step_.errMsg();
		    return res;
		}

    Step&		step_;
    mutable int		totalnr_;
    uiString		msg_;
};

} // namespace VolProc


VolProc::Step::Step()
    : chain_( 0 )
    , output_( 0 )
    , id_( cUndefID() )
{
    inputs_.setNullAllowed();
}


VolProc::Step::~Step()
{
    deepUnRef( inputs_ );
}


void VolProc::Step::resetInput()
{
    deepUnRef( inputs_ );

    inputslotids_.setEmpty();
    inputcompnrs_.setEmpty();
    for ( int idx=0; idx<getNrInputs(); idx++ )
    {
	if ( inputs_.validIdx(idx) )
	    inputs_.replace( idx, 0 );
	else
	    inputs_ += 0;

	inputslotids_ += idx;
	inputcompnrs_ += mUdf(int);
    }
}


void VolProc::Step::releaseData()
{
    output_ = 0;

    resetInput();
}


VolProc::Chain& VolProc::Step::getChain()
{
    return *chain_;
}

const VolProc::Chain& VolProc::Step::getChain() const
{
    return const_cast<Step*>(this)->getChain();
}


void VolProc::Step::setChain( VolProc::Chain& c )
{
    if ( chain_ )
    {
	pErrMsg("Can only add to chain once");
	return;
    }

    chain_ = &c;
    if ( mIsUdf(id_) )
	id_ = c.getNewStepID();
}


const char* VolProc::Step::userName() const
{
    return username_.isEmpty() ? 0 : username_.buf();
}


void VolProc::Step::setUserName( const char* nm )
{
    username_ = nm;
}


int VolProc::Step::getNrInputs() const
{
    return isInputPrevStep() ? 1 : 0;
}


int VolProc::Step::getInputSlotID( int idx ) const
{
    if ( !isInputPrevStep() && !needsInput() )
	return cUndefSlotID();

    if ( idx<0 || idx>=getNrInputs() )
    {
	pErrMsg("Invalid input slot");
	return cUndefSlotID();
    }

    return inputslotids_.validIdx(idx) ? inputslotids_[idx] : idx;
}


void VolProc::Step::getInputSlotName( InputSlotID slotid,
					BufferString& res ) const
{
    res.set( "Input " ).add( slotid );
}


int VolProc::Step::getOutputSlotID( int idx ) const
{
    if ( idx<0 || idx>=getNrOutputs() )
	{ pErrMsg("Invalid output slot"); return cUndefSlotID(); }

    return idx;
}


bool VolProc::Step::validInputSlotID( InputSlotID slotid ) const
{
    for ( int idx=0; idx<getNrInputs(); idx++ )
    {
	if ( getInputSlotID(idx)==slotid )
	    return true;
    }

    return false;
}


bool VolProc::Step::validOutputSlotID( OutputSlotID slotid ) const
{
    for ( int idx=0; idx<getNrOutputs(); idx++ )
    {
	if ( getOutputSlotID(idx)==slotid )
	    return true;
    }

    return false;
}



void VolProc::Step::setInput( InputSlotID slotid,
			      const RegularSeisDataPack* dc )
{
    if ( inputs_.isEmpty() )
    {
	TypeSet<int> inputcompnrs( inputcompnrs_ );
	resetInput();
	inputcompnrs_ = inputcompnrs;
    }

    const int idx = inputslotids_.indexOf( slotid );
    if ( !inputs_.validIdx(idx) )
	return;

    unRefPtr( inputs_[idx] );
    inputs_.replace( idx, dc );
    refPtr( inputs_[idx] );
}


VolProc::Step::CVolRef VolProc::Step::getInput( InputSlotID slotid ) const
{
    const int idx = inputslotids_.indexOf( slotid );
    return inputs_.validIdx(idx) ? inputs_[idx] : 0;
}


void VolProc::Step::setOutput( OutputSlotID slotid, RegularSeisDataPack* dc )
{
    output_ = dc;
}


VolProc::Step::VolRef VolProc::Step::getOutput( OutputSlotID slotid )
{
    // TODO: implement using slotid
    return output_;
}


VolProc::Step::CVolRef VolProc::Step::getOutput( OutputSlotID slotid ) const
{
    return const_cast<Step*>(this)->getOutput( slotid );
}


void VolProc::Step::enableOutput( OutputSlotID slotid )
{
    outputslotids_.addIfNew( slotid );
}


int VolProc::Step::getOutputIdx( OutputSlotID slotid ) const
{
    return outputslotids_.indexOf( slotid );
}


void VolProc::Step::fillPar( IOPar& par ) const
{
    if ( !username_.isEmpty() )
	par.set( sKey::Name(), username_.buf() );

    par.set( sKey::ID(), id_ );
}


bool VolProc::Step::usePar( const IOPar& par )
{
    username_.empty();
    par.get( sKey::Name(), username_ );
    if ( !par.get(sKey::ID(),id_) && chain_ )
	id_ = chain_->getNewStepID();

    return true;
}


bool VolProc::Step::prepareWork( int nrthreads )
{
    if ( !output_ )
    {
	errmsg_ = tr( "Volume Processing step does not have an output" );
	releaseData();
	return false;
    }

    const Pos::GeomID geomid = output_->sampling().hsamp_.getGeomID();
    if ( needsInput() )
    {
	for ( int idx=0; idx<getNrInputs(); idx++ )
	{
	    if ( !inputs_.validIdx(idx) || !inputs_[idx] )
	    {
		errmsg_ = tr( "Volume Processing step does not have the "
			      "required input datapack" );
		return false;
	    }

	    const InputSlotID slotid = getInputSlotID( idx );
	    if ( inputs_[idx]->nrComponents() !=
					getNrInputComponents(slotid,geomid) )
	    {
		errmsg_ = tr( "Volume Processing step does not have the "
				"required number of components for its input" );
		releaseData();
		return false;
	    }
	}
    }

    const OutputSlotID slotid = getOutputSlotID( 0 );
    const bool success = output_->nrComponents() ==
					getNrOutComponents( slotid, geomid );
    if ( !success )
    {
	errmsg_ = tr( "Volume Processing step does not have the "
			"required number of components for its output" );
    }

    return success;
}


ReportingTask* VolProc::Step::createTask()
{
    if ( areSamplesIndependent() && prefersBinIDWise() )
	return new BinIDWiseTask( *this );

    return 0;
}


int VolProc::Step::getNrInputComponents( InputSlotID slotid,
					 Pos::GeomID geomid ) const
{
    if ( !validInputSlotID(slotid) )
	return 0;

    if ( needsInput() && slotid >=0 && slotid < getNrInputs() )
	return inputcompnrs_.validIdx(slotid) && !mIsUdf(inputcompnrs_[slotid])
		 ? inputcompnrs_[slotid] : 1;

    return 0;
}


int VolProc::Step::getNrOutComponents( OutputSlotID slotid, Pos::GeomID ) const
{
    return validOutputSlotID(slotid) && slotid == 0 ?
	( outputcompnrs_.validIdx(slotid) && !mIsUdf(outputcompnrs_[slotid])
	  ? outputcompnrs_[slotid] : 1 ) : 0;
}


void VolProc::Step::setInputNrComps( InputSlotID slotid, int nrcomps )
{
    if ( !validInputSlotID(slotid) )
	return;

    if ( !inputcompnrs_.validIdx(slotid) )
    {
	for ( int idx=0; idx<=slotid; idx++ )
	{
	    if ( !inputcompnrs_.validIdx(idx) )
		inputcompnrs_ += mUdf(int);
	}
    }

    inputcompnrs_[slotid] = nrcomps;
    OutputSlotID outslotid;
    if ( !copyComponentsSel(slotid,outslotid) )
	return;

    if ( !outputcompnrs_.validIdx(outslotid) )
    {
	for ( int idx=0; idx<=outslotid; idx++ )
	{
	    if ( !outputcompnrs_.validIdx(idx) )
		outputcompnrs_ += mUdf(int);
	}
    }

    outputcompnrs_[outslotid] = nrcomps;
}


TrcKeyZSampling VolProc::Step::getInputSampling(
					     const TrcKeyZSampling& tkzs ) const
{
    TrcKeyZSampling res( tkzs );
    res.hsamp_.expand( getHorizontalStepout().inl(),
		       getHorizontalStepout().crl() );
    res.zsamp_.widen( res.zsamp_.step * getVerticalStepout() );

    const Survey::Geometry* geom = Survey::GM().getGeometry(
						    res.hsamp_.getGeomID() );
    if ( geom )
	res.limitTo( geom->sampling(), true );

    return res;
}


od_uint64 VolProc::Step::getComponentMemory( const TrcKeyZSampling& tkzs,
					     bool input ) const
{
    const TrcKeyZSampling usedtkzs = input ? getInputSampling( tkzs ) : tkzs;

    return usedtkzs.totalNr() * sizeof(float);
}


od_int64 VolProc::Step::getExtraMemoryUsage( const TrcKeyZSampling& tkzs,
					const TypeSet<OutputSlotID>& ids ) const
{
    TypeSet<OutputSlotID> tocalc( ids );
    if ( tocalc.isEmpty() )
    {
	const int nroutputs = getNrOutputs();
	for ( int idx=0; idx<nroutputs; idx++ )
	    tocalc += getOutputSlotID( idx );
    }

    od_int64 ret = 0;
    for ( int idx=0; idx<tocalc.size(); idx++ )
	ret += extraMemoryUsage( tocalc[idx], tkzs );

    return ret;
}



class NullTracesArrayScanner : public ParallelTask
{ mODTextTranslationClass(NullTracesArrayScanner);
public:

		NullTracesArrayScanner( const Array3DImpl<float>& data,
					const TrcKeySampling& tks,
					const PosInfo::CubeData* trcssampling )
		    : ParallelTask("Null Traces array scanner")
		    , data_(data)
		    , tks_(tks)
		    , trcssampling_(trcssampling)
		    , totalnr_(tks.totalNr())
		    , result_(0)
		{}
		~NullTracesArrayScanner()
		{
		    deepErase( detectors_ );
		    delete result_;
		}

    uiString	message() const		{ return uiString::empty(); }
    uiString	nrDoneText() const	{ return ParallelTask::sTrcFinished(); }

    const PosInfo::CubeData*	getResult() const	{ return result_; }

protected:

    od_int64	nrIterations() const	{ return totalnr_; }

private:

    bool	doPrepare( int nrthreads )
		{
		    deleteAndZeroPtr( result_ );
		    deepErase( detectors_ );
		    const PosInfo::Detector::Setup setup( false );
		    for ( int idx=0; idx<nrthreads; idx++ )
			detectors_ += new PosInfo::Detector( setup );

		    return true;
		}

    bool	doWork(od_int64 start, od_int64 stop, int threadid )
		{
		    if ( !detectors_.validIdx(threadid) )
			return false;

		    PosInfo::Detector& detector = *detectors_[threadid];
		    const bool isrect = !trcssampling_ ||
					trcssampling_->isFullyRectAndReg();
		    const TrcKeySampling tks( tks_ );
		    const float* dataptr = data_.getData();
		    const ValueSeries<float>* datastor = data_.getStorage();
		    const bool hasarrayptr = dataptr;
		    const bool hasstorage = datastor;
		    const bool neediterate = !dataptr && !datastor;
		    const Array3DInfo& info = data_.info();
		    const int nrz = info.getSize(2);
		    const od_int64 offset = start * nrz;
		    if ( hasarrayptr ) dataptr += offset;
		    od_uint64 validx = hasstorage ? offset : 0;
		    Array2DInfoImpl hinfo( info.getSize(0), info.getSize(1) );
		    ArrayNDIter* hiter = 0;
		    if ( neediterate )
		    {
			hiter = new ArrayNDIter( hinfo );
			hiter->setGlobalPos( start );
		    }

		    for ( od_int64 idx=start; idx<=stop; idx++,
							 quickAddToNrDone(idx) )
		    {
			if ( !isrect && !trcssampling_->isValid(idx,tks) )
			{
			    if ( hasarrayptr ) dataptr += nrz;
			    else if ( hasstorage ) validx += nrz;
			    else hiter->next();
			    continue;
			}


			bool allnull = true;
			for ( int idz=0; idz<nrz; idz++ )
			{
			    const float val = hasarrayptr ? dataptr[idz]
				: hasstorage ? datastor->value( validx+idz )
				 : data_.get( (*hiter)[0], (*hiter)[1], idz );
			    if ( val != 0.f )
			    {
				allnull = false;
				break;
			    }
			}

			if ( hasarrayptr )	dataptr+=nrz;
			else if ( hasstorage )	validx +=nrz;
			else if ( hiter )	hiter->next();

			if ( allnull )
			    continue;

			const TrcKey tk( tks.atIndex( idx ) );
			detector.add( tk.getCoord(), tk.position() );
		    }

		    delete hiter;

		    return detector.finish() && detector.usable();
		}

    bool	doFinish( bool )
		{
		    for ( int idx=1; idx<detectors_.size(); idx++ )
			detectors_[0]->appendResults( *detectors_[idx] );

		    PosInfo::CubeData* result = new PosInfo::CubeData;
		    const PosInfo::Detector& detector =
			const_cast<const PosInfo::Detector&>( *detectors_[0] );
		    detector.getCubeData( *result );
		    result_ = result;

		    return true;
		}

    const Array3DImpl<float>&	data_;
    const TrcKeySampling&	tks_;
    const PosInfo::CubeData*	trcssampling_;
    const od_int64	totalnr_;
    ObjectSet<PosInfo::Detector>	detectors_;
    const PosInfo::CubeData*	result_;
};


const PosInfo::CubeData* VolProc::Step::getPosSamplingOfNonNullTraces(
					InputSlotID id, int compidx )
{
    if ( !validInputSlotID(id) )
	return 0;

    CVolRef input = getInput( id );
    if ( !input || !input->validComp(compidx) )
	return 0;

    NullTracesArrayScanner scanner( input->data( compidx ),
				    input->sampling().hsamp_,
				    input->trcsSampling() );
    return scanner.execute() && scanner.getResult()
		? new PosInfo::SortedCubeData( *scanner.getResult() ) : 0;
}
