// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/pmap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

void
showmappings(uintptr_t start, uintptr_t end){
    extern pde_t *kern_pgdir;
    int off = ~0xfff;
    cprintf("Memory mappings from  0x%08x to 0x%08x\n", start, end);
    for (uintptr_t i = start; i < end; i += PGSIZE) {
        pte_t* pte = pgdir_walk(kern_pgdir, (void*)i, 0);
        if (pte == NULL)
            cprintf("virtual address  0x%08x  could not be mapped to a physical address\n", i);
        else
            cprintf("virtual address  0x%08x  physical address  0x%08x permissions:  PTE_P  %x  PTE_W  %x  PTE_U  %x\n", i, *pte & off, *pte & PTE_P, *pte & PTE_W, *pte & PTE_U);
    }
}
void
setpermission(void* address, char permissions[])
{
    pte_t *pte = pgdir_walk(kern_pgdir, address, 0);
    uint32_t perm = 0;
    if (strchr(permissions, 'P'))
        perm |= PTE_P;
    if (strchr(permissions, 'W'))
        perm |= PTE_W;
    if (strchr(permissions, 'U'))
        perm |= PTE_U;
    if (strchr(permissions, '0'))
        perm = 0;
    *pte = *pte | perm;
    showmappings((uintptr_t)address, (uintptr_t)address+PGSIZE);
}

void
memorydump(uintptr_t start, uintptr_t end)
{
    extern pde_t *kern_pgdir;
    uintptr_t addr[2], i, j;
    cprintf("Contents from virtual address 0x%08x to virtual address 0x%08x\n", start, end);
    for (i = start; i < end; i += 1) {
        j = ROUNDDOWN(i, PGSIZE);
        pte_t* pte = pgdir_walk(kern_pgdir, (void*)j, 0);
        if (!pte)
            cprintf("0x%08x: contents are not mapped here\n", i);
        else {
             cprintf("0x%08x:    0x%08x\n", i, *(uint32_t*)i);
        }
    }
}

void
print_art(){
cprintf("\x1b[31m #######                                                                   ###\x1b[0m \n");
cprintf("\x1b[32m #       #    # ##### #####    ##       ####  #####  ###### #####  # ##### ###\x1b[0m \n");
cprintf("\x1b[33m #        #  #    #   #    #  #  #     #    # #    # #      #    # #   #   ###\x1b[0m \n");
cprintf("\x1b[34m #####     ##     #   #    # #    #    #      #    # #####  #    # #   #    #\x1b[0m  \n");
cprintf("\x1b[35m #         ##     #   #####  ######    #      #####  #      #    # #   #     \x1b[0m \n");
cprintf("\x1b[36m #        #  #    #   #   #  #    #    #    # #   #  #      #    # #   #   ###\x1b[0m \n");
cprintf("\x1b[31m ####### #    #   #   #    # #    #     ####  #    # ###### #####  #   #   ###\x1b[0m \n");
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    // Your code here.
  struct Eipdebuginfo eip_dbg;
  int frames = 0; // Tracks the stack frame count
  int args[5];
  int ebp = read_ebp(), eip = read_ebp() + 4; //Gets base pointer and stack pointer based on the position on the base pointer

  for(int i = 0; i<5; i++){
    args[i] = *((int*)(ebp + (8 + (4 * i)))); //Collects the 5 arguments by the offset from ebp starting at 8 and incrementing by 4
  }

  while (ebp!=0) // While the base pointer is not zero
  {
    if (debuginfo_eip(*((int*)eip), &eip_dbg) == -1)//check for an error
        cprintf("BACKTRACE DEBUGGING ERROR\n");

    // Prints the base pointer, stack pointer, and arguments specified in the directions
    cprintf("ebp %08x  eip %08x  args %08x %08x %08x %08x %08x \n",ebp, *((int*)eip), args[0], args[1], args[2], args[3], args[4]);

    int eip_offset = *((int*)eip)-eip_dbg.eip_fn_addr;
    // Prints the function name, source file name, and line number corresponding to the eip
    cprintf("%s:%d: %.*s+%d\n", eip_dbg.eip_file, eip_dbg.eip_line, eip_dbg.eip_fn_namelen, eip_dbg.eip_fn_name, eip_offset);

    ebp = *((int*)ebp); // Moves ebp

    eip = ebp+4; // Moves eip based on epb's move above

    for(int i = 0; i < 5; i++){
         args[i]= *((int*)(ebp + (8 + (4 * i))));  //Collects the 5 arguments by the offset from ebp starting at 8 and incrementing by 4
     }
    frames++; // Increment the stack frame count
  }
  return frames; // return the stack frame count
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
        if (strcmp(argv[0], "art") == 0)
            print_art();
        if (strcmp(argv[0],"showmappings") == 0 && argc == 3)
            showmappings(ROUNDDOWN(strtol(argv[1], NULL, 16), 4096), ROUNDDOWN(strtol(argv[2], NULL, 16), 4096));
        if (strcmp(argv[0],"memorydump") == 0 && argc == 3)
            memorydump(ROUNDDOWN(strtol(argv[1], NULL, 16), 4096), ROUNDDOWN(strtol(argv[2], NULL, 16), 4096));
        if (strcmp(argv[0],"setpermissions") == 0 && argc == 3)
            setpermission((void*)strtol(argv[1], NULL, 0), argv[2]);
        return 0;
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");
    cprintf("\x1b[35m Type 'art' to see some art for EXTRA CREDIT! \x1b[0m    \n");
    cprintf("\x1b[36m ex usage: 'showmappings 0xffefffff 0xffffffff' for EXTRA CREDIT! \x1b[0m    \n");
    cprintf("\x1b[37m ex usage: 'memorydump 0xffefffff 0xffffffff' for EXTRA CREDIT! \x1b[0m    \n");
    cprintf("\x1b[31m ex usage: 'setpermissions 0xffefffff W' for EXTRA CREDIT! \x1b[0m    \n");
    if(tf != NULL)
        print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
