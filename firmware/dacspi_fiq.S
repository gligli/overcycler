#define T0IR 0xE0004000
#define T0MR0 0xE0004018
#define SSPDR 0xE0068008

.data 

.align 5
.global dacspi_states
dacspi_states:
	// see dacspi.h for meaning
	// max 16 voices
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0
	.word 0,0,0,0,0,0,0,0

	// termination
	.word 0,0,0,0,0,0,0,0

.global dacspi_master_period
dacspi_master_period:
	.word 0

.text
.arm

.global FIQ_Init
FIQ_Init:
	ldr r12,=dacspi_states
	ldr r11,=T0IR
	ldr r10,=SSPDR
	ldr r9,=dacspi_master_period
	mov r8,#500
	bx lr

.global FIQ_Handler
FIQ_Handler:
	
	// save regs
	stmfd sp!,{r0-r4}

	// ack int
	mov r2,#1
	strb r2,[r11]

	// load osc state
	ldr r0,[r12]
	
	// past last osc?
	cmp r0,#0
	bne dovoice

	str r8,[r12,#24]

	// restart from first osc 
	ldr r12,=dacspi_states

	// load next state
	ldr r0,[r12]

dovoice:

	// load osc state
	ldr r1,[r12,#4]
	ldr r4,[r12,#16]

	// update osc counter
	subs r1,r8
	bgt dosample

	// reload counter
	ldr r3,[r12,#12]
	add r1,r3

	// update phase
	ldr r3,[r12,#20]
	subs r4,r3
	bge dosample

	// reload phase
	ldr r3,[r12,#8]
	add r4,r3

	// get master period
	ldr r8,[r9]
	
dosample:
	
	// update osc state
	str r1,[r12,#4]
	str r4,[r12,#16]

	// get sample command
	mov r4,r4,lsl#1
	ldrh r4,[r0,r4]

	// send it
	strh r4,[r10]

	// to next osc
	add r12,#32

	// restore regs
	ldmfd sp!, {r0-r4}

	// return
	subs pc, lr, #4

