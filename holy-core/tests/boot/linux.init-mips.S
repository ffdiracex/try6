#define SYSCALL_WRITE 4004
#define SYSCALL_RESET 4088
#define SYSCALL_EXIT 4001

#define STDOUT 1
#define SHUTDOWN_MAGIC1 0xfee1dead
#define SHUTDOWN_MAGIC2 0x28121969
#ifdef REBOOT
#define SHUTDOWN_MAGIC3 0x01234567
#else
#define SHUTDOWN_MAGIC3 0x4321fedc
#endif
	.text
	.global start, _start, __start
__start:	
_start:
start:
	/* write.  */
	li $v0, SYSCALL_WRITE
	li $a0, STDOUT
	lui $a1, %hi(message)
	addiu $a1, %lo(message)
	lui $a2, %hi(messageend)
	addiu $a2, %lo(messageend)
	subu $a2, $a2, $a1
	syscall

	/* shutdown.  */
	li $v0, SYSCALL_RESET
	li $a0, SHUTDOWN_MAGIC1
	li $a1, SHUTDOWN_MAGIC2
	li $a2, SHUTDOWN_MAGIC3
	syscall

	/* exit(1). Shouldn't be reached.  */
	li $v0, SYSCALL_EXIT
	li $a0, 1
	syscall

	.data
message:
	.ascii "Boot Test Passed Successfully\n" SUCCESSFUL_BOOT_STRING "\n"
messageend:
