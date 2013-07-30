#define T0IR 0xE0004000
#define PWMTC 0xE0014008
#define SSPDR 0xE0068008

.data 

.align 5
.global dacspi_states
dacspi_states:
	// sample count / waveform data address / counter / period / phase / increment / unused / unused
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

.text
.arm

.global FIQ_Init
FIQ_Init:
	ldr r12,=dacspi_states
	ldr r11,=T0IR
	ldr r10,=SSPDR
	ldr r9,=PWMTC
	bx lr

.global FIQ_Handler
FIQ_Handler:
	
	// ack int
	mov r8,#1
	strb r8,[r11]

	// save regs
	stmfd sp!,{r0-r7}

	// load osc state
	ldmia r12,{r0-r7}
	
	// past last osc?
	cmp r0,#0
	bne dovoice

	// restart from first osc 
	ldr r12,=dacspi_states
	// load osc state
	ldmia r12,{r0-r7}

dovoice:
	// get elapsed ticks
	mov r8,r6
	ldr r6,[r9]
	sub r8,r6,r8

	// update osc counter
	subs r2,r8
	bge dosample

	// reload counter
	add r2,r3

	// update phase
	subs r4,r5
	bge dosample

	// reload phase
	add r4,r0

dosample:
	
	// update osc state
	stmia r12!,{r0-r7}

	// get sample
	mov r8,r4,lsl#1
	ldrh r8,[r1,r8]

	// send it
	strh r8,[r10]

	// restore regs
	ldmfd sp!, {r0-r7}

	// return
	subs pc, lr, #4

