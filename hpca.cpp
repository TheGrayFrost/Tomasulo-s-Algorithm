#include <iostream>
#include <string>
#include <deque>

/***********************************************************************************
*************************   CONSTANTS AND TYPE DEFINITIONS     *********************
***********************************************************************************/


#define ADD 0		// opcode for add
#define SUB 1		// opcode for subtract
#define MUL 2		// opcode for multiply
#define DIV 3		// opcode for divide

#define ADDCYCLE 2	// add takes 2 cycles
#define SUBCYCLE 2	// subtract takes 2 cycles
#define MULCYCLE 10	// multiply takes 10 cycles
#define DIVCYCLE 40	// divide takes 40 cycles

#define INSCOUNT 10	// number of instructions in instruction queue
#define REGCOUNT 8	// number of registers

#define ADDCOUNT 3	// number of addition reservation stations
#define MULCOUNT 2	// number of multiplication reservation stations

#define OPCODE int	// opcodes are integers
#define REG int		// register values are integers
#define RLOC int	// register locations are stored in RAT as integers

#define EMPTY -1	// if RAT must point to RF directly, its value is set to empty
					// also used if some value need not be waited on in RS

/***********************************************************************************
***************************    STRUCTURE DEFINITIONS     ***************************
***********************************************************************************/


// instruction structure
struct INS
{
	OPCODE op;	// opcode
	RLOC dst;	// destination reg
	RLOC src1;	// source 1
	RLOC src2;	// source 2

	INS() {op = -1; dst = src1 = src2 = EMPTY;}
	void print();
};

// reservation station entry
struct RSE
{
	// data
	bool busy;	// occupied or not
	OPCODE op;	// opcode
	REG Vj;		// value 1, if available
	REG Vk;		// value 2, if available
	RLOC Qj;	// value 1 source, if waiting for it
	RLOC Qk;	// value 2 source, if waiting for it
	bool disp;	// whether dispatched

	// execution information
	int discno;	// cycle in which instruction was dispatched
	int bcast;	// cycle in which its result will be broadcast
	REG result;	// result on operation completion

	RSE() {busy = false; discno = -1;}
	void doOp();
	void print();
};

/***********************************************************************************
****************************    STRUCTURE FUNCTIONS     ****************************
***********************************************************************************/


// opcode to string
std::string OpName (OPCODE op)
{
	switch (op)
	{
		case ADD: return "Add";
		case SUB: return "Sub";
		case MUL: return "Mul";
		case DIV: return "Div";
		default: return "Invalid";
	}
}

void INS::print()
{
	std::cout << OpName(op) << " R" << dst << ", R";
	std::cout << src1 << ", R" << src2 << "\n";
}

void RSE::doOp()
{
	switch (op)
	{
		case ADD: result = Vj + Vk; return;
		case SUB: result = Vj - Vk; return;
		case MUL: result = Vj * Vk; return;
		case DIV: result = Vj / Vk; return;
		default: result = -1; return;
	}
}

void RSE::print()
{
	std::cout << "\t" << busy << "\t";
	if (!busy)
		std::cout << "\t\t\t\t\t\t\n";
	else
	{
		std::cout << OpName(op) << "\t";
		if (Qj == EMPTY)
			std::cout << Vj;
		std::cout << "\t";
		if (Qk == EMPTY)
			std::cout << Vk;
		std::cout << "\t";
		if (Qj != EMPTY)
			std::cout << "RS" << Qj;
		std::cout << "\t";
		if (Qk != EMPTY)
			std::cout << "RS" << Qk;
		std::cout << "\t" << disp << "\n";
	}
}

/***********************************************************************************
*****************************    HARDWARE ELEMENTS     *****************************
***********************************************************************************/


INS * INSMEM;			// Instruction Memory (holds complete program)
int PROGSIZE;			// Program size
int CURINS;				// Instructions seen uptil now from memory

int CYCLE;				// Current cycle no

std::deque<INS> IQUEUE;	// Instruction Queue
REG RF[REGCOUNT];		// Register File

RLOC RAT[REGCOUNT];		// RAT
RSE ADDRS[ADDCOUNT];	// Add Reservation Stations
RSE MULRS[MULCOUNT];	// Multiply Reservation Stations

/***********************************************************************************
****************************    EXECUTION FUNCTIONS     ****************************
***********************************************************************************/


// fill instruction queue from the memory
void fillIQ ()
{
	// until all instructions have been read, or the instruction queue is full
	while (CURINS != PROGSIZE && IQUEUE.size() != INSCOUNT)
		IQUEUE.push_back(INSMEM[CURINS++]);	// keep filling in the instructions
}

// get cycles taken for an operation
int opCycles (OPCODE op)
{
	switch(op)
	{
		case ADD: return ADDCYCLE;
		case SUB: return SUBCYCLE;
		case MUL: return MULCYCLE;
		case DIV: return DIVCYCLE;
		default: return -1;
	}
}

// dispatch any instructions whose values are available
void dispatch()
{
	// iterate over the add reservation stations in the dispatch priority order
	for (int i = 0; i < ADDCOUNT; ++i)
	{
		// check if any undispatched instruction has both values available
		if (ADDRS[i].busy && !ADDRS[i].disp 
				&& ADDRS[i].Qj == EMPTY && ADDRS[i].Qk == EMPTY)
		{
			ADDRS[i].disp = true;		// mark it as dispatched
			ADDRS[i].discno = CYCLE;	// dispatched in the current cycle
			// result will be broadcast when operation completes
			ADDRS[i].bcast = CYCLE + opCycles(ADDRS[i].op);	
			break;
		}
	}

	// same logic for multiply reservation stations
	for (int i = 0; i < MULCOUNT; ++i)
	{
		if (MULRS[i].busy && !MULRS[i].disp
				&& MULRS[i].Qj == EMPTY && MULRS[i].Qk == EMPTY)
		{
			MULRS[i].disp = true;
			MULRS[i].discno = CYCLE;
			MULRS[i].bcast = CYCLE + opCycles(MULRS[i].op);
			break;
		}
	}
}

// fill an RS entry with instruction ins
void fillRS (RSE& rs, INS ins)
{
	rs.busy = true;				// declare it busy
	rs.op = ins.op;				// set the opcode
	if (RAT[ins.src1] == EMPTY)	// if src1 is available in RF
		rs.Vj = RF[ins.src1];	// get it from there
	if (RAT[ins.src2] == EMPTY)	// if src2 is available in RF
		rs.Vk = RF[ins.src2];	// get it from there
	rs.Qj = RAT[ins.src1];		// otherwise wait for Vj as per RAT
	rs.Qk = RAT[ins.src2];		// otherwise wait for Vk as per RAT
	rs.disp = false;			// not yet dispatched
}

// get a reservation station for an instruction
RLOC getRS (INS ins)
{
	if (ins.op == ADD || ins.op == SUB)			// for add and subtract
	{
		for (int i = 0; i < ADDCOUNT; ++i)		// check all add stations
		{
			// if anyone is not busy and was not freed in current cycle
			if (!ADDRS[i].busy && ADDRS[i].discno != CYCLE)
			{
				fillRS(ADDRS[i], ins);			// fill it with the instruction
				return i;						// return it
			}
		}
	}

	else if (ins.op == MUL || ins.op == DIV)	// same idea for multiply and divide
	{
		for (int i = 0; i < MULCOUNT; ++i)
		{
			if (!MULRS[i].busy && MULRS[i].discno != CYCLE)
			{
				fillRS(MULRS[i], ins);
				return i + ADDCOUNT;
			}
		}
	}

	return -1;	// if not available return -1
}

// issue an instruction from front of instruction queue
void issue()
{
	if (!IQUEUE.empty())
	{
		INS nextins = IQUEUE.front();		// take the next ins in IQUEUE
		int rsalloc = getRS(nextins);		// get an RS for it
		if (rsalloc != -1)					// if getRS succeeded
		{
			RAT[nextins.dst] = rsalloc;		// update RAT
			IQUEUE.pop_front();				// remove the instruction from queue
		}
	}
}

// capture the result res from reservation station rsn
void capture (int res, int rsn)
{
	// for each add reservation station
	for (int i = 0; i < ADDCOUNT; ++i)
	{
		if (ADDRS[i].Qj == rsn)		// if Qj was waiting for rsn
		{
			ADDRS[i].Vj = res;		// feed the result into Vj
			ADDRS[i].Qj = EMPTY;	// set value Vj as available
		}
		if (ADDRS[i].Qk == rsn)		// same thing for Qk
		{
			ADDRS[i].Vk = res;
			ADDRS[i].Qk = EMPTY;
		}
	}
	// do the same thing for multiplication reservation stations
	for (int i = 0; i < MULCOUNT; ++i)
	{
		if (MULRS[i].Qj == rsn)
		{
			MULRS[i].Vj = res;
			MULRS[i].Qj = EMPTY;
		}
		if (MULRS[i].Qk == rsn)
		{
			MULRS[i].Vk = res;
			MULRS[i].Qk = EMPTY;
		}
	}

	// update RAT and RF
	for (int i = 0; i < REGCOUNT; ++i)
	{
		if (RAT[i] == rsn)	// if any reg value was to be filled from rsn
		{
			RAT[i] = EMPTY;	// set that reg value as available
			RF[i] = res;	// fill that register's value
		}
	}
}

// stall all stations ready to broadcast in current cycle for one cycle
void stallOthers()
{
	// among all add stations
	for (int i = 0; i < ADDCOUNT; ++i)
		// if any is ready to broadcast in current cycle
		if (ADDRS[i].busy && ADDRS[i].disp && ADDRS[i].bcast == CYCLE)
			// then for all the stations
			for (int j = 0; j < ADDCOUNT; ++j)
				// if any others are in the same operation pipeline 
				if (ADDRS[j].op == ADDRS[i].op)
					// delay all of their broadcasts by one cycle
					ADDRS[j].bcast++;

	// same logic for multiplication stations
	for (int i = 0; i < MULCOUNT; ++i)
		if (MULRS[i].busy && MULRS[i].disp && MULRS[i].bcast == CYCLE)
			for (int j = 0; j < MULCOUNT; ++j)
				if (MULRS[j].op == MULRS[i].op)
					MULRS[j].bcast++;
}

// broadcast the value when available
void broadcast()
{
	// go over the multiplication stations first (higher broadcast priority)
	for (int i = 0; i < MULCOUNT; ++i)
	{
		// if any was gets its result in the current cycle
		if (MULRS[i].busy && MULRS[i].disp && MULRS[i].bcast == CYCLE)
		{
			MULRS[i].doOp();							// get the result
			capture (MULRS[i].result, i + ADDCOUNT);	// capture the generated result
			MULRS[i].bcast = -1;						// set it as broadcasted
			stallOthers();								// stall any others if need be
			MULRS[i].busy = false;						// free the station
			break;
		}
	}

	// same logic for add station broadcasts
	for (int i = 0; i < ADDCOUNT; ++i)
	{
		if (ADDRS[i].busy && ADDRS[i].disp && ADDRS[i].bcast == CYCLE)
		{
			ADDRS[i].doOp();
			capture (ADDRS[i].result, i);
			ADDRS[i].bcast = -1;
			stallOthers();
			ADDRS[i].busy = false;
			break;
		}
	}
}

void ExecCycle()
{
	dispatch();				// dispatch some instructions if possible
	issue();				// try issuing an instruction
	fillIQ();				// fill instruction queue if possible
	broadcast();			// broadcast any results if available
	CYCLE++;
}

/***********************************************************************************
******************************   DISPLAY FUNCTIONS     *****************************
***********************************************************************************/


// display reservation stations
void showRS()
{
	std::cout << "\tBusy\tOp\tVj\tVk\tQj\tQk\tDisp\n";
	for (int i = 0; i < ADDCOUNT; ++i)
	{
		std::cout << "RS" << i;
		ADDRS[i].print();
	}
	for (int i = 0; i < MULCOUNT; ++i)
	{
		std::cout << "RS" << ADDCOUNT + i;
		MULRS[i].print();
	}
}

// display RF and RAT
void showREG()
{
	std::cout << "\tRF\tRAT\n";
	for (int i = 0; i < REGCOUNT; ++i)
	{
		std::cout << i << ":\t" << RF[i];
		if (RAT[i] != EMPTY)
			std::cout << "\tRS" << RAT[i];
		std::cout << "\n";
	}
}

// display instruction queue
void showIQ()
{
	std::cout << "Instruction Queue\n\n";
	for (auto ins: IQUEUE)
		ins.print();
}

// display complete hardware state
void showState()
{
	std::cout << "\n-------------------------- STATE ---------------------------\n";
	std::cout << "------------------------------------------------------------\n";
	showRS();
	std::cout << "\n------------------------------------------------------------\n";
	showREG();
	std::cout << "\n------------------------------------------------------------\n";
	showIQ();
	std::cout << "\n------------------------------------------------------------\n";
	std::cout << "------------------------------------------------------------\n";
}

/***********************************************************************************
***************************   MAIN: INPUT AND DRIVERS     **************************
***********************************************************************************/


int main()
{
	int target;

	// take input
	std::cin >> PROGSIZE;				// Instructions in Program
	INSMEM = new INS [PROGSIZE];		// Allocate space for program memory
	std::cin >> target;					// Number of cycles to simulate
	for (int i = 0; i < PROGSIZE; ++i)	// Get all instructions into memory
		std::cin >> INSMEM[i].op >> INSMEM[i].dst >> INSMEM[i].src1 >> INSMEM[i].src2;
	for (int i = 0; i < REGCOUNT; ++i)	// Initialise Register File with given values
		std::cin >> RF[i];
	for (int i = 0; i < REGCOUNT; ++i)	// Initialise RAT to all EMPTY
		RAT[i] = EMPTY;

	// run for required number of cycles
	while (CYCLE <= target)
		ExecCycle();

	// show state
	showState();
}



