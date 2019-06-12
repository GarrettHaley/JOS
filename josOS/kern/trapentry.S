/* See COPYRIGHT for copyright information. */

# include <inc/mmu.h>
# include <inc/memlayout.h>
# include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
  Lab 3: Your code here for generating entry points for the different traps.
 */
NOEC(T_DIVIDE_H, T_DIVIDE)
NOEC(T_DEBUG_H, T_DEBUG)
NOEC(T_NMI_H, T_NMI)
NOEC(T_BRKPT_H, T_BRKPT)
NOEC(T_OFLOW_H, T_OFLOW)
NOEC(T_BOUND_H, T_BOUND)
NOEC(T_ILLOP_H, T_ILLOP)
NOEC(T_DEVICE_H, T_DEVICE)
TRAPHANDLER(T_DBLFLT_H, T_DBLFLT)
TRAPHANDLER(T_TSS_H, T_TSS)
TRAPHANDLER(T_SEGNP_H, T_SEGNP)
TRAPHANDLER(T_STACK_H, T_STACK)
TRAPHANDLER(T_GPFLT_H, T_GPFLT)
TRAPHANDLER(T_PGFLT_H, T_PGFLT)
NOEC(T_FPERR_H, T_FPERR)
TRAPHANDLER(T_ALIGN_H, T_ALIGN)
NOEC(T_MCHK_H, T_MCHK)
NOEC(T_SIMDERR_H, T_SIMDERR)
NOEC(T_SYSCALL_H, T_SYSCALL)
NOEC(T_DEFAULT_H, T_DEFAULT)
NOEC(irq0, IRQ_OFFSET+0 );
NOEC(irq1, IRQ_OFFSET+1 );
NOEC(irq2, IRQ_OFFSET+2 );
NOEC(irq3, IRQ_OFFSET+3 );
NOEC(irq4, IRQ_OFFSET+4 );
NOEC(irq5, IRQ_OFFSET+5 );
NOEC(irq6, IRQ_OFFSET+6 );
NOEC(irq7, IRQ_OFFSET+7 );
NOEC(irq8, IRQ_OFFSET+8 );
NOEC(irq9, IRQ_OFFSET+9 );
NOEC(irq10, IRQ_OFFSET+10 );
NOEC(irq11, IRQ_OFFSET+11 );
NOEC(irq12, IRQ_OFFSET+12 );
NOEC(irq13, IRQ_OFFSET+13 );
NOEC(irq14, IRQ_OFFSET+14 );
NOEC(irq15, IRQ_OFFSET+15 );

/*
  Lab 3: Your code here for _alltraps
 */
_alltraps:
    pushl %ds
    pushl %es
    pushal
    pushl $GD_KD
    popl %ds
    pushl $GD_KD
    popl %es
    pushl %esp
    call trap

