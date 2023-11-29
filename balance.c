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
#define BALANCED_LOAD_THRESHOLD 0.01 //percent 2, 1, 0.1, 0.01

struct Processor *first = NULL;
unsigned long global_cycles = 0;
unsigned long totalLoad = 0;
unsigned balancedLoad = 0;
int iterations = 0;

int getUniformlyRandomLoadUnits(){
    return rand() % (L_MAX - L_MIN + 1) + L_MIN;
}

int getUniformlyRandomNextActivityTime(){
    return rand() % (D_MAX - D_MIN + 1) + D_MIN;
}

struct Processor{
    int position;
    int loadUnits;
    int nextLoadBalanceTime;
    //left neighor
    struct Processor *left;
    //right neighor
    struct Processor *right;
};

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

    balancedLoad = totalLoad * (BALANCED_LOAD_THRESHOLD / 100);
    if(balancedLoad > totalLoad / k)
        balancedLoad = balancedLoad/10;
    if(balancedLoad < 1)
        balancedLoad = 1;
}

bool isLoadDistributedUniformly()
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

void balanceLoad(struct Processor *proc)
{
    unsigned long total = proc->loadUnits + proc->left->loadUnits + proc->right->loadUnits;
    unsigned long averageLoadUnits = total/3;

    if(proc->loadUnits > averageLoadUnits)
    {
        unsigned giveAway = proc->loadUnits - averageLoadUnits;
        if (proc->left->loadUnits < averageLoadUnits) 
        {
            int needs = averageLoadUnits - proc->left->loadUnits;
            int intransit = giveAway < needs ? giveAway : needs;
            proc->left->loadUnits  += intransit;
            proc->loadUnits -= intransit;
            giveAway -= intransit;
        }

        if (proc->right->loadUnits < averageLoadUnits) 
        {
            int needs = averageLoadUnits - proc->right->loadUnits;
            int intransit = giveAway < needs ? giveAway : needs;
            proc->right->loadUnits  += intransit;
            proc->loadUnits -= intransit;
            giveAway -= intransit;
        }
    }
}

void performLoadBalancingActivity(int k)
{
    struct Processor *curr = first;
    
    while(global_cycles < MAX_CYCLES && !isLoadDistributedUniformly())
    {
        iterations++;
        balanceLoad(curr);
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

    initializeRingSystem(k);
    if(verbose)
    {
        printf("System Configuration before load balancing:\n");
        printAllProcs();

        printf("\nTotal Load = %lu\n", totalLoad);
        printf("Max Balanced Load Difference = %d\n\n", balancedLoad);
    }

    printf("\nTotal Load = %lu\n", totalLoad);
        printf("Max Balanced Load Difference = %d\n\n", balancedLoad);

    performLoadBalancingActivity(k);
    
    if(verbose)
    {
        printf("System Configuration after load balancing:\n");
        printAllProcs();
    }

    printf("\nTime Cycles = %lu\n", global_cycles);
    printf("Total load balancing activitied = %d\n", iterations);

    
    return 0;
}
