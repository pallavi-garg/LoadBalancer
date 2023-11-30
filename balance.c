#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define L_MIN 10
#define L_MAX 1000
#define D_MIN 100
#define D_MAX 1000 
#define MAX_CYCLES 1000000 // Max number of cycles to run 
#define BALANCED_LOAD_THRESHOLD 0.01 //percent 5.0, 2.0, 1.0, 0.1, 0.5, 0.01
#define RUNS 5

//first proc in the ring system
struct Processor *first = NULL;
unsigned long global_cycles = 0;
unsigned long totalLoad = 0;
unsigned balancedLoad = 0;
int iterations = 0;

/*
Returns random number of load units
*/
int getUniformlyRandomLoadUnits(){
    return rand() % (L_MAX - L_MIN + 1) + L_MIN;
}

/*
Returns random next time cycle for running load balancing activity
*/
int getUniformlyRandomNextActivityTime(){
    return rand() % (D_MAX - D_MIN + 1) + D_MIN;
}

/*
Represents processor in the system
*/
struct Processor{
    int position;
    int loadUnits;
    int nextLoadBalanceTime;
    //left neighor
    struct Processor *left;
    //right neighor
    struct Processor *right;
};

/*
Returns next processor starting from given proc's right neighor,
who has lowest nextLoadBalanceTime value.
*/
struct Processor* getNextProcessor(struct Processor *proc)
{
    struct Processor *curr = proc->right, *next = proc->right;
    while(curr != NULL && curr != proc)
    {
        if(next->nextLoadBalanceTime > curr->nextLoadBalanceTime)
            next = curr;
        curr = curr->right;
    }
    return next;
}

/*
Returns new processor node with given position
*/
struct Processor* createNewProcessor(int position)
{
    struct Processor *proc = malloc(sizeof(struct Processor));
    proc->position = position;
    //create unbalanced load units in the begining
    proc->loadUnits = position % 3 == 0 ? getUniformlyRandomLoadUnits() : 0;
    totalLoad += proc->loadUnits;
    proc->nextLoadBalanceTime = getUniformlyRandomNextActivityTime();
    proc->left = NULL;
    proc->right = NULL;
    return proc;
}

/*
Helper function to print all the processors with their assigned Load Units
*/
void printAllProcs()
{
    printf("Position = %d, Load = %d\n", first->position, first->loadUnits);
    struct Processor *curr = first->right;
    while(curr != NULL && curr != first)
    {
        printf("Position = %d, Load = %d\n", curr->position, curr->loadUnits);
        curr = curr->right;
    }
}

/*
Ring system initializer
*/
void initializeRingSystem(int k)
{
    first = createNewProcessor(0);
    struct Processor *prev = first, *newProc = NULL;

    for(size_t i = 1; i < k; i++)
    {
        newProc = createNewProcessor(i);
        if(prev != NULL)
        {
            prev->right = newProc;
            newProc->left = prev;
        }
        prev = newProc;
    }
    if(newProc != NULL)
    {
        newProc->right = first;
        first->left = newProc;
    }

    balancedLoad = totalLoad * (BALANCED_LOAD_THRESHOLD / 100.0);
    if(balancedLoad > totalLoad / k)
        balancedLoad = balancedLoad/10;
    if(balancedLoad < 1)
        balancedLoad = 1;
}

void disposeRingSystem(int k)
{
    struct Processor *curr = first->right, *prev = first;
    while(curr != NULL && curr != first)
    {
        prev->right = NULL;
        struct Processor *next = curr->right;
        free(curr);
        next->left = NULL;
        curr = next;
    }
    free(first);
    first = NULL;

    global_cycles = 0;
    totalLoad = 0;
    balancedLoad = 0;
    iterations = 0;
}

/*
Returns true if steady state is achieved in the system.
Steady state means all the load is distributed uniformly.
This is calculated using global knowledge.
*/
bool isSteadyStateAchieved()
{
    struct Processor *curr = first;
    do
    {
        if((curr->left != NULL && abs(curr->loadUnits - curr->left->loadUnits) > balancedLoad)
            || (curr->right != NULL && abs(curr->loadUnits - curr->right->loadUnits) > balancedLoad))
        {
            return false;
        }
        curr = curr->right;
    } while(curr != NULL && curr != first);

    return true;
}

/*
Shifts the load from proc to given neighbor proc
*/
bool shiftLoad(struct Processor *proc, struct Processor *neighbor, unsigned long averageLoadUnits)
{
    bool isLoadShifted = false;
    if (neighbor->loadUnits < averageLoadUnits)
    {
        unsigned extraLoadUnits = proc->loadUnits - averageLoadUnits;
        int capacityToTakeLoadUnits = averageLoadUnits - neighbor->loadUnits;
        int shifts = extraLoadUnits < capacityToTakeLoadUnits ? extraLoadUnits : capacityToTakeLoadUnits;
        neighbor->loadUnits  += shifts;
        proc->loadUnits -= shifts;
        extraLoadUnits -= shifts;
    }
    return isLoadShifted;
}

/*
Balances load of given processor by using local knowledge of neighboring processors
*/
bool balanceLoad(struct Processor *proc)
{
    bool isLoadShifted = false;

    unsigned long total = proc->loadUnits;

    if(proc->left != NULL)
        total += proc->left->loadUnits;
    if(proc->right != NULL)
        total += proc->right->loadUnits;

    unsigned long averageLoadUnits = total/3;

    if(proc->loadUnits > averageLoadUnits)
    {
        if(proc->left != NULL)
            isLoadShifted = shiftLoad(proc, proc->left, averageLoadUnits);
        if(proc->right != NULL)
            isLoadShifted = shiftLoad(proc, proc->right, averageLoadUnits) || isLoadShifted;
    }
    return isLoadShifted;
}

/*
Main loop in which keeps tracks of time cycles and checks if steady state is achieved or not
*/
void performLoadBalancing(int k)
{
    struct Processor *curr = first;
    bool isLoadShifted = true;
    
    while(global_cycles < MAX_CYCLES && (isLoadShifted || !isSteadyStateAchieved()))
    {
        iterations++;
        isLoadShifted = balanceLoad(curr);
        global_cycles = curr->nextLoadBalanceTime;
        curr->nextLoadBalanceTime += getUniformlyRandomNextActivityTime();
        curr = getNextProcessor(curr);
    }
}

int main(int argc, char **argv){
    if (argc < 2) 
    {
        printf("Usage: %s <k>\n", argv[0]);
        printf("k    : Number of processors in the system and k should be > 0\n");
        printf("Additionaly 'v' can be used to show the verbose results. Usage: %s <k> v\n", argv[0]);
        exit(1);
    }
    int k = atoi(argv[1]);
    if(k <= 0)
    {
        printf("k should be > 0\n");
        exit(1);
    }
    bool verbose = false;
    srand(time(0)); //Init seed for random numbers

    if(argc == 3)
    {
        if(*argv[2] == 'v' ||  *argv[2] == 'V')
            verbose = true;
        else{
            printf("Second argument can be either 'v' or 'V'.\n");
            exit(1);
        }
    }

    unsigned long long totalCycles = 0;
    unsigned long long totalIterations = 0;
    unsigned long long totalSystemLoad = 0;

    for(int i = 1; i <= RUNS; i++)
    {
        printf("----Run %d----\n", i);
        initializeRingSystem(k);
        if(verbose)
        {
            printf("System Configuration before load balancing:\n");
            printAllProcs();
            printf("\nMax Balanced Load Difference = %d\n", balancedLoad);
        }

        performLoadBalancing(k);
        
        if(verbose)
        {
            printf("\nSystem Configuration after load balancing:\n");
            printAllProcs();
            printf("\n");
        }

        printf("Total Load = %lu\n", totalLoad);
        printf("Time Cycles = %lu\n", global_cycles);
        printf("Total load balancing activities = %d\n", iterations);

        totalSystemLoad += totalLoad;
        totalCycles += global_cycles;
        totalIterations += iterations;

        disposeRingSystem(k);
        printf("\n");
    }

    printf("\n**** Averaged Data *****\n");
    printf("Average Load = %llu\n", totalSystemLoad/RUNS);
    printf("Average Time Cycles = %llu\n", totalCycles/RUNS);
    printf("Average Total load balancing activities = %llu\n\n", totalIterations/RUNS);

    //printf("%f, %d, %llu, %llu, %llu\n", BALANCED_LOAD_THRESHOLD, k, totalSystemLoad/RUNS, totalCycles/RUNS, totalIterations/RUNS);
    
    return 0;
}
