#include "dacspi_fiq.h"

#define T0 0xE0004000
#define SSPDR 0xE0068008
#define FIO0 0x3FFFC000
#define EXTINT 0xE01FC140
.data 

.align 5
.global dacspi_states
dacspi_states:
	// see dacspi_fiq.h for meaning
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

.macro doOsc osc

	// test osc int flag
	tst r2,#(1<<\osc)
	beq 9f

	// get osc state address
	ldr r3,=dacspi_states
	add r3,r3,#(32*\osc)

	// load state
	ldmia r3,{r4-r7}

	// update state
	subs r5,r7
	addlt r5,r6

	// save state
	str r5,[r3,#(DSIDX_PHASE*4)]
	
	// advance timer match
	ldr r7,[r12,#(0x18+\osc*4)]
	ldr r9,[r3,#(DSIDX_PERIOD*4)]
	add r9,r7
	str r9,[r12,#(0x18+\osc*4)]

	// get sample
	mov r9,r5,lsl#1
	ldrh r6,[r4,r9]

	// send sample
	strh r6,[r10]

	// wait until SSP has sent the command
1:
	ldrb r9,[r10,#4]
	tst r9,#16
	bne 1b

	// send /LDAC pulse (load sample into its voice)
	mov r8,#(0x10<<\osc)
	strb r8,[r11,#(0x1c+1)]
	nop
	nop
	nop
	nop
	nop
	nop
	strb r8,[r11,#(0x18+1)]

9:

.endm


.global FIQ_Init
FIQ_Init:
	ldr r12,=T0
	ldr r11,=FIO0
	ldr r10,=SSPDR
	mov r8,#0
	bx lr

.global FIQ_Handler
FIQ_Handler:

	// save regs
	stmfd sp!,{r2-r7}

	// ack ints
	ldrb r2,[r12]
	strb r2,[r12]

	// handle oscs
	doOsc 0
	doOsc 1
	doOsc 2
	doOsc 3

	// restore regs
	ldmfd sp!, {r2-r7}

	// return
	subs pc, lr, #4

