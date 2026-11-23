/*! \file
 *  \brief Scheduling module for the OS.
 *
 * Contains everything needed to realise the scheduling between multiple processes.
 * Also contains functions to start the execution of programs.
 *
 *  \author Lehrstuhl Informatik 11 - RWTH Aachen
 */

#include "os_scheduler.h"

#include "lcd.h"
#include "os_core.h"
#include "os_input.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "util.h"

#include <avr/interrupt.h>
#include <stdbool.h>

//----------------------------------------------------------------------------
// Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//! Array of states for every possible process
#error IMPLEMENT STH. HERE
Process os_processes[MAX_NUMBER_OF_PROCESSES]; 

//! Index of process that is currently executed (default: idle)
#warning IMPLEMENT STH. HERE
ProcessID currentProc = 0; 
//----------------------------------------------------------------------------
// Private variables
//----------------------------------------------------------------------------

//! Currently active scheduling strategy
#warning IMPLEMENT STH. HERE
SchedulingStrategy currentStrategy = OS_SS_EVEN;

//! Count of currently nested critical sections
#warning IMPLEMENT STH. HERE
uint8_t criticalSectionCount = 0; 
//----------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------

//! ISR for timer compare match (scheduler)
ISR(TIMER2_COMPA_vect)
__attribute__((naked));

//----------------------------------------------------------------------------
// Function definitions
//----------------------------------------------------------------------------

/*!
 *  Timer interrupt that implements our scheduler. Execution of the running
 *  process is suspended and the context saved to the stack. Then the periphery
 *  is scanned for any input events. If everything is in order, the next process
 *  for execution is derived with an exchangeable strategy. Finally the
 *  scheduler restores the next process for execution and releases control over
 *  the processor to that process.
 */
ISR(TIMER2_COMPA_vect) {
#warning IMPLEMENT STH. HERE
    saveContext();
    // Aktuellen wert von sp im Prozess-Array speichern
    os_processes[currentProc].sp.as_int = SP;
    SP = BOTTOM_OF_ISR_STACK;

    os_processes[currentProc].checksum = os_getStackChecksum(currentProc);
    // ENTER + ESC überprüfen
    if ((os_getInput() & 0b10000001) == 0b1000001) {

        os_waitForNoInput(); //losslassen der tasten

        // TaskManager starten
        os_taskManMain();
    }
    // soll wieder ausgeführt werden
    os_processes[currentProc].state = OS_PS_READY;

    // Muss entscheiden welcher der bereiten Prozesse als nächstes ausgeführt wird
    switch(currentStrategy) {
        // ausgeführt, wenn even stragety aktiv ist
        case OS_SS_EVEN:
            currentProc = os_Scheduler_Even(os_processes, currentProc);
            break;
        
        // ausgeführt wenn random stragety aktiv ist
        case OS_SS_RANDOM:
            currentProc = os_Scheduler_Random(os_processes, currentProc);
            break;
        
        default: 
            currentProc = os_Scheduler_Even(os_processes, currentProc);
            break; 

    }

    if(os_processes[currentProc].checksum != os_getStackChecksum(currentProc)) {
        os_error("Stack checksum mismatch");
    }
    // ausgewählten prizess auf running setzen
    os_processes[currentProc].state = OS_PS_RUNNING;

    // Stackpointer des neuen prozesses wiederhertsellen
    SP = os_processes[currentProc].sp.as_int;

    restoreContext();
}

/*!
 *  This is the idle program. The idle process owns all the memory
 *  and processor time no other process wants to have.
 */
void idle(void){
#warning IMPLEMENT STH. HERE
    while(1) {
        lcd_writeChar('.');
        delayMs(DEFAULT_OUTPUT_DELAY); 

    }
}

/*!
 *  This function is used to register the given program for execution.
 *  A stack will be provided if the process limit has not yet been reached.
 *  This function is multitasking safe. That means that programs can repost
 *  themselves, simulating TinyOS 2 scheduling (just kick off interrupts ;) ).
 *
 *  \param program  The function of the program to start.
 *  \param priority A priority ranging 0..255 for the new process:
 *                   - 0 means least favourable
 *                   - 255 means most favourable
 *                  Note that the priority may be ignored by certain scheduling
 *                  strategies.
 *  \return The index of the new process or INVALID_PROCESS as specified in
 *          defines.h on failure
 */
ProcessID os_exec(Program *program, Priority priority) {
#error IMPLEMENT STH. HERE

    if (program == NULL) {
        return INVALID_PROCESS; 
    }

    // enter critical section 
    os_enterCriticalSection();

    for (uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
        if (os_processes[i].state == OS_PS_UNUSED) {
            os_processes[i].program = program; 
            os_processes[i].priority = priority; 


            // Stack pointer as pointer --> sp.as_ptr 
            // mit * ist der inshalt der speicheradressed gemeint, ohne ist der zeiger also die speicheraddresse

            StackPointer sp; 
            sp.as_int = PROCESS_STACK_BOTTOM(i); 

            uint16_t address = (uint16_t)program;

            // Low. bvyte zuerst
            *sp.as_ptr = (uint8_t)(address & 0b11111111);
            sp.as_int--; 

            // Jetzt der High byte
            *sp.as_ptr = (uint8_t)(address >> 8); 
            sp.as_int--;

            for (uint8_t j = 0; j < 33; j++) {
                *sp.as_ptr = 0; 
                sp.as_int--;
            }

            // Nächste Stakposition speichern
            os_processes[i].sp = sp; 

            os_processes[i].checksum = os_getStackChecksum(i);

            // Prozess bereit zur ausführung
            os_processes[i].state = OS_PS_READY;

            // leave critical section
            os_leaveCriticalSection();
            return i; 
        }
    }

    os_leaveCriticalSection();

    return INVALID_PROCESS;
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
#warning IMPLEMENT STH. HERE
    currentProc = 0; 
    os_processes[currentProc].state = OS_PS_RUNNING;
    SP = os_processes[currentProc].sp.as_int;

    restoreContext();
}

/*!
 *  In order for the Scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register.
 */
void os_initScheduler(void){
#warning IMPLEMENT STH. HERE
    for (uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
        os_processes[i].state = OS_PS_UNUSED;

    }

    os_exec(idle, DEFAULT_PRIORITY); 

    // autostart programm starten
    struct program_linked_list_node *current = autostart_head; 

    while (current != NULL) {
        os_exec(current->program, DEFAULT_PRIORITY);
        // von jetzigen programm auf nächstes springen
        current = current->next;
    }
}

/*!
 *  A simple getter for the slot of a specific process.
 *
 *  \param pid The processID of the process to be handled
 *  \return A pointer to the memory of the process at position pid in the os_processes array.
 */
Process *os_getProcessSlot(ProcessID pid) {
    return os_processes + pid;
}

/*!
 *  A simple getter to retrieve the currently active process.
 *
 *  \return The process id of the currently active process.
 */
ProcessID os_getCurrentProc(void) {
#warning IMPLEMENT STH. HERE
    return currentProc;
}

/*!
 *  Sets the current scheduling strategy.
 *
 *  \param strategy The strategy that will be used after the function finishes.
 */
void os_setSchedulingStrategy(SchedulingStrategy strategy){
#warning IMPLEMENT STH. HERE
    currentStrategy = strategy;
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
#warning IMPLEMENT STH. HERE
    return currentStrategy;
}

/*!
 *  Enters a critical code section by disabling the scheduler if needed.
 *  This function stores the nesting depth of critical sections of the current
 *  process (e.g. if a function with a critical section is called from another
 *  critical section) to ensure correct behaviour when leaving the section.
 *  This function supports up to 255 nested critical sections.
 */
void os_enterCriticalSection(void) {
#warning IMPLEMENT STH. HERE
    uint8_t oldSREG = SREG;
    SREG &= 0b01111111;  // der interrupt wird deaktiviert --> so wird der kritische punkt nciht abgebrochen
    
    if (criticalSectionCount == 255) {
        SREG = (SREG & 0b01111111) | (oldSREG & 0b10000000);

        os_error("Critical section overflow");

        return; 
    }
    criticalSectionCount++; 

    // Sscheduler deaktivieren 
    TIMSK2 &= ~(1 << OCIE2A); 

    // Interrupt wiederherstellen
    SREG = (SREG & 0b01111111) | (oldSREG & 0b10000000);
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void){
#warning IMPLEMENT STH. HERE
    uint8_t oldSREG = SREG; 
    SREG &= 0b01111111; // interrupt wird deaktiviert aber kritischer punkt wird nicht aabgebrochen

    if (criticalSectionCount == 0) {
        SREG = (SREG & 0b01111111) | (oldSREG & 0b10000000);

        os_error("No critical section"); 
        return; 
    }

    criticalSectionCount--;

    // scheduler nur beim letzen verlassen aktivieren
    if (criticalSectionCount == 0 ) {
        TIMSK2 |= (1 << OCIE2A); // Scheduler aktivieren

    }

    SREG = (SREG & 0b01111111) | (oldSREG & 0b10000000);
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
#warning IMPLEMENT STH. HERE
    StackChecksum checksum = 0; 

    // höhste adresse prozessstacks
    
    uint16_t stackBottom = PROCESS_STACK_BOTTOM(pid);

    // gespeciherter stackpointer
    uint16_t stackPointer = os_processes[pid].sp.as_int; 

    for (uint16_t address = stackBottom; address > stackPointer; address--) {

        checksum ^= *((uint8_t *)address);

    }
    return checksum;
}
