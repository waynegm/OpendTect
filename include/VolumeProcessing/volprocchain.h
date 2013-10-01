#ifndef volprocchain_h
#define volprocchain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		October 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "volumeprocessingmod.h"
#include "attribdatacubes.h"
#include "multiid.h"
#include "executor.h"
#include "factory.h"
#include "refcount.h"
#include "samplingdata.h"
#include "threadlock.h"

namespace Attrib { class DataCubes; }

class VelocityDesc;
class Executor;
class HorSampling;
template <class T> class StepInterval;
template <class T> class ValueSeries;

namespace VolProc
{

class Chain;
class StepExecutor;
class StepTask;
    
/*!
 \brief An algorithm/calculation/transformation that takes one scalar volume as
 input, processes it, and puts the output in another volume.
 */

mExpClass(VolumeProcessing) Step
{
public:
				typedef int ID;
				typedef int SlotID;
    
				mDefineFactoryInClass( Step, factory );
    virtual			~Step();
    
    static ID			cUndefID()		{ return mUdf(int); }
    static SlotID		cUndefSlotID()		{ return mUdf(int); }
    ID				getID() const		{ return id_; }
    
    Chain&			getChain()		{ return *chain_; }
    void			setChain(Chain&);
    
    virtual const char*		userName() const;
    virtual void		setUserName(const char* nm);
    
    virtual bool		needsInput() const		= 0;
    virtual int			getNrInputs() const;
    virtual SlotID		getInputSlotID(int idx) const;
    virtual int			getNrOutputs() const		{ return 1; }
    virtual SlotID		getOutputSlotID(int idx) const;
    bool			validInputSlotID(SlotID) const;
    bool			validOutputSlotID(SlotID) const;


    virtual HorSampling		getInputHRg(const HorSampling&) const;
				/*!<When computing HorSampling, how
				 big input is needed? */
    virtual StepInterval<int>	getInputZRg(const StepInterval<int>&) const;
				/*!<When computing HorSampling, how
				 big input is needed?*/
    
    virtual void		setInput(SlotID,const Attrib::DataCubes*);
    virtual void		setOutput(Attrib::DataCubes*, //add SlotID
				      const HorSampling&,
				      const StepInterval<int>&);
    
    int				getOutputIdx(SlotID) const;
    void			enableOutput(SlotID);
    
    virtual bool		canInputAndOutputBeSame() const { return false;}
    virtual bool		needsFullVolume() const { return true; }
    const Attrib::DataCubes*	getOutput() const	{ return output_; }
    Attrib::DataCubes*		getOutput()		{ return output_; }
    
    virtual const VelocityDesc*	getVelDesc() const	{ return 0; } // old
    
    virtual bool		areSamplesIndependent() const { return true; }
				/*!<returns whether samples in the output
				 are independent from each other.*/
    
    virtual Task*		createTask();
    
    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);
    
    virtual void		releaseData();
    
    virtual const char*		errMsg() const { return 0; }
    
protected:
				Step();
    
    friend		class BinIDWiseTask;
    virtual bool	prefersBinIDWise() const		{ return false;}
    virtual bool	computeBinID(const BinID&,int threadid)	{ return false;}
    virtual bool	prepareComp(int nrthreads)		{ return true;}
    
    Chain*			chain_;
    
    Attrib::DataCubes*		output_;
    const Attrib::DataCubes*	input_;
    BufferString		username_;
    ID				id_;
    
    HorSampling			hrg_;
    StepInterval<int>		zrg_;
    TypeSet<SlotID>		outputslotids_; // enabled slotids
};



/*!
\brief A chain of Steps that can be applied to a volume of scalars.
*/

mExpClass(VolumeProcessing) Chain
{ mRefCountImpl(Chain);
public:
    				Chain();

    mExpClass(VolumeProcessing) Connection
    {
    public:
				Connection( Step::ID outpstep=Step::cUndefID(),
				    Step::SlotID outpslot=Step::cUndefSlotID(),
				    Step::ID inpstep=Step::cUndefID(),
				    Step::SlotID inpslot=Step::cUndefSlotID());
	
	bool			isUdf() const;
	bool			operator==(const Connection&) const;
	bool			operator!=(const Connection&) const;
	
	void			fillPar(IOPar&,const char* key) const;
	bool			usePar(const IOPar&,const char* key);

	Step::ID		outputstepid_;
	Step::SlotID		outputslotid_;
	Step::ID		inputstepid_;
	Step::SlotID		inputslotid_;
	
    };
    
    mExpClass(VolumeProcessing) Web
    {
    public:
	bool			addConnection(const Connection&);
	void			removeConnection(const Connection&);
	void			getConnections(Step::ID,bool input,
					       TypeSet<Connection>&) const;
				/*!Gets all connection that has step as either
				   input or output. */
	
	TypeSet<Connection>&		getConnections()
					{ return connections_; }
	const TypeSet<Connection>&	getConnections() const
					{ return connections_; }
    private:
	TypeSet<Connection>	connections_;
    };
    
    bool			addConnection(const Connection&);
    void			removeConnection(const Connection&);
    const Web&			getConnections() const	{ return connections_; }
    
    void			setZStep( float z, bool zist )
				{ zstep_=z; zist_ = zist; }
    float			getZStep() const	{ return zstep_; }
    bool			zIsT() const		{ return zist_; }

    int				nrSteps() const; 
    Step*			getStep(int);
    Step*			getStepFromName(const char*);
    const Step*			getStepFromName(const char*) const;
    Step*			getStepFromID(Step::ID);
    const Step*			getStepFromID(Step::ID) const;
    int				indexOf(const Step*) const;
    void			addStep(Step*);
    void			insertStep(int,Step*);
    void			swapSteps(int,int);
    void			removeStep(int);
    const ObjectSet<Step>&	getSteps() const	{ return steps_; }
    
    bool			setOutputSlot(Step::ID,Step::SlotID) const;

    const VelocityDesc*		getVelDesc() const;

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    bool			setOutputSlot(Step::ID,Step::SlotID);
    void			setStorageID(const MultiID& mid);
    const MultiID&		storageID() const { return storageid_; }

    bool			areSamplesIndependent() const;
    bool			needsFullVolume() const;
    const char*			errMsg() const;

    Step::ID			getNewStepID() { return freeid_++; }
    
private:
    friend			class ChainExecutor;
    
    bool			validConnection(const Connection&) const;

    static const char*		sKeyNrSteps()	    { return "Nr Steps"; }
    static const char*		sKeyStepType()	    { return "Type"; }
    static const char*		sKeyNrConnections() { return "Nr Connections"; }
    static const char*		sKeyConnection(int idx,BufferString&);
    
    Step::ID			outputstepid_;
    Step::SlotID		outputslotid_;
    
    MultiID			storageid_;
    ObjectSet<Step>		steps_;
    Web				connections_;

    float			zstep_;
    bool			zist_;

    BufferString		errmsg_;
    Threads::Atomic<int>	freeid_;
};



/*!
\brief Chain Executor
*/

mExpClass(VolumeProcessing) ChainExecutor : public Executor
{
public:
				ChainExecutor(Chain&);
				~ChainExecutor();

    const char*			errMsg() const;

    bool			setCalculationScope(const HorSampling&,
						    const StepInterval<int>&);
    
    const Attrib::DataCubes*	getOutput() const;
    int				nextStep();

private:
    class Epoch 
    {
    public:
				Epoch(const ChainExecutor& c)
				    : taskgroup_( *new TaskGroup )
				    , chainexec_( c )
				{ taskgroup_.setParallel(true); }
	
				~Epoch()		{ delete &taskgroup_; }
	
	void			addStep(Step* s)	{ steps_ += s; }
	
	bool			doPrepare();
	Task&			getTask()		{ return taskgroup_; }
	
	bool			needsStepOutput(Step::ID) const;
	
    private:
	
	BufferString		errmsg_;
	const ChainExecutor&	chainexec_;
	TaskGroup&		taskgroup_;
	ObjectSet<Step>		steps_;
    };
    
    bool			scheduleWork();
    int				computeLatestEpoch(Step::ID) const;
    void			computeComputationScope(Step::ID stepid,
				    HorSampling& stepoutputhrg,
				    StepInterval<int>& stepoutputzrg ) const;

    void			controlWork(Task::Control);
    od_int64			nrDone() const;
    od_int64			totalNr() const;
    const char*			message() const;
    
    void			releaseMemory();
    
    Task*			curtask_;

    bool			isok_;
    bool			firstisprep_;
    Chain&			chain_;

    HorSampling			outputhrg_;
    StepInterval<int>		outputzrg_;

    mutable BufferString	errmsg_;
    ObjectSet<Step>		scheduledsteps_;
    ObjectSet<Epoch>		epochs_;
    Chain::Web			connections_;
    int				totalnrepochs_;
    
    const Attrib::DataCubes*	outputvolume_;
};

}; // namespace VolProc

#endif

