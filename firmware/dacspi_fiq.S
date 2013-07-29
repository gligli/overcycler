#define OC_VOICE_COUNT 2
#define OC_OSC_COUNT (OC_VOICE_COUNT*2)

.text
.arm

.global dacspi_commands
dacspi_commands:
	.hword 0,0,0,0
	.hword 0,0,0,0
	.hword 0,0,0,0
	.hword 0,0,0,0

.global FIQ_Init
FIQ_Init:
	ldr r12,dacspi_commands
	ldr r11,=0xe0068008 // SSPDR
	mov r8,#0	
	bx lr

.global FIQ_Handler
FIQ_Handler:
	ldr r9,[r12,r8,LSL#1]
	strh r9,[r11]
	add r8,#1

	ldr r9,[r12,r8,LSL#1]
	strh r9,[r11]
	add r8,#1

	ldr r9,[r12,r8,LSL#1]
	strh r9,[r11]
	add r8,#1

	ldr r9,[r12,r8,LSL#1]
	strh r9,[r11]
	add r8,#1

	cmp r8,#OC_OSC_COUNT
	subnes pc, lr, #4

	mov r8,#0	
	stmfd sp!,{r0-r12,lr}
	bl dacspi_advanceInterrupt
	ldmfd sp!, {r0-r12,lr}

	subs pc, lr, #4

