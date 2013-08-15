#include "dacspi_fiq.h"

#define T0 0xE0004000
#define SSPDR 0xE0068008
#define FIO 0x3FFFC000
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

.global dacspi_ref
dacspi_ref:
	.word 0

.text
.arm

#define SCK_BIT 5
#define SDI_BIT 6

.macro bbOne bit

	and r4,r9,r5,lsr#(15-\bit)
	strb r4,[r11,#(0x34+3)]
	strb r8,[r11,#(0x38+3)]
	
.endm

.macro bbSPI valr sselv

	mov r9,#\sselv
	strb r9,[r11,#(0x1c+1)]

	mov r5,\valr,lsl#SDI_BIT
	mov r9,#(1<<SDI_BIT)
	mov r8,#(1<<SCK_BIT)

	bbOne 0
	bbOne 1
	bbOne 2
	bbOne 3
	bbOne 4
	bbOne 5
	bbOne 6
	bbOne 7
	bbOne 8
	bbOne 9
	bbOne 10
	bbOne 11
	bbOne 12
	bbOne 13
	bbOne 14
	bbOne 15
	
	mov r9,#\sselv
	strb r9,[r11,#(0x18+1)]

.endm

.macro doOsc osc

	// test osc int flag
	tst r2,#(1<<(\osc&3))
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
	ldr r7,[r12,#(0x18+(\osc&3)*4)]
	ldr r9,[r3,#(DSIDX_PERIOD*4)]
	add r9,r7
	str r9,[r12,#(0x18+(\osc&3)*4)]

	// get sample
	mov r9,r5,lsl#1
	ldrh r6,[r4,r9]

	bbSPI r6,(0x80>>(\osc>>1))

9:

.endm


.global FIQ_Init
FIQ_Init:
	ldr r12,=T0
	ldr r11,=FIO
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

	// go to timer1
	add r12,#0x4000

	// ack ints
	ldrb r2,[r12]
	strb r2,[r12]

	// handle oscs
	doOsc 4
	doOsc 5
	doOsc 6
	doOsc 7

	// back to timer0
	sub r12,#0x4000

	// send reference
	ldr r4,=dacspi_ref
	ldrh r4,[r4]
	bbSPI r4,0x04

	// restore regs
	ldmfd sp!, {r2-r7}

	// return
	subs pc, lr, #4

