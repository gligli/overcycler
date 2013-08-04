#define T0IR 0xE0004000
#define T0MR0 0xE0004018
#define SSPDR 0xE0068008
#define FIO0 0x3FFFC000
#define EXTINT 0xE01FC140
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
	str r5,[r3,#4]

	// get sample
	ldr r7,[r4,r5,lsl#1]

	// advance timer match
	ldr r6,[r9,#(\osc*4)]
	ldr r4,[r3,#20]
	add r6,r4
	str r6,[r9,#(\osc*4)]


	// wait until SSP ready
1:
	ldrb r4,[r10,#4]
	tst r4,#16
	bne 1b


	// send /LDAC pulse (load last sample into its voice)
	strb r8,[r11,#(0x1c+1)]
	nop
	nop
	nop
	nop
	nop
	nop
	strb r8,[r11,#(0x18+1)]

	// send new sample
	strh r7,[r10]

	// save voice mask for future sample load
	mov r8,#(0x10<<\osc)

9:

.endm


.global FIQ_Init
FIQ_Init:
	ldr r12,=T0IR
	ldr r11,=FIO0
	ldr r10,=SSPDR
	ldr r9,=T0MR0
	mov r8,#0
	bx lr

.global FIQ_Handler
FIQ_Handler:

	// save regs
	stmfd sp!,{r2-r7}

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

