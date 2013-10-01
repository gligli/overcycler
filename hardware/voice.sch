EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:voice-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Overcycler voice board schematics"
Date "1 oct 2013"
Rev "1"
Comp "GliGli's DIY"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L SSM2044 U5
U 1 1 5208A055
P 7700 3150
F 0 "U5" H 7700 3050 50  0000 C CNN
F 1 "SSM2044" H 7700 3250 50  0000 C CNN
F 2 "MODULE" H 7700 3150 50  0001 C CNN
F 3 "DOCUMENTATION" H 7700 3150 50  0001 C CNN
	1    7700 3150
	1    0    0    -1  
$EndComp
$Comp
L TL074 U4
U 4 1 5208A05B
P 5700 4400
F 0 "U4" H 5750 4600 60  0000 C CNN
F 1 "TL074" H 5850 4200 50  0000 C CNN
F 2 "" H 5700 4400 60  0000 C CNN
F 3 "" H 5700 4400 60  0000 C CNN
	4    5700 4400
	1    0    0    -1  
$EndComp
$Comp
L 4051 U1
U 1 1 5208A061
P 2650 3900
F 0 "U1" H 2750 3900 60  0000 C CNN
F 1 "4051" H 2750 3700 60  0000 C CNN
F 2 "" H 2650 3900 60  0000 C CNN
F 3 "" H 2650 3900 60  0000 C CNN
	1    2650 3900
	-1   0    0    -1  
$EndComp
$Comp
L MCP4922-E/P U2
U 1 1 5208A067
P 2850 2550
F 0 "U2" H 3000 2950 40  0000 L BNN
F 1 "MCP4922-E/P" H 3000 2900 40  0000 L BNN
F 2 "DIP14" H 2850 2550 30  0000 C CIN
F 3 "" H 2850 2550 60  0000 C CNN
	1    2850 2550
	1    0    0    -1  
$EndComp
$Comp
L MCP4922-E/P U3
U 1 1 5208A06D
P 2850 5450
F 0 "U3" H 3000 5850 40  0000 L BNN
F 1 "MCP4922-E/P" H 3000 5800 40  0000 L BNN
F 2 "DIP14" H 2850 5450 30  0000 C CIN
F 3 "" H 2850 5450 60  0000 C CNN
	1    2850 5450
	1    0    0    -1  
$EndComp
$Comp
L C C12
U 1 1 5208A079
P 4350 4350
F 0 "C12" H 4350 4450 40  0000 L CNN
F 1 "100n" H 4356 4265 40  0000 L CNN
F 2 "~" H 4388 4200 30  0000 C CNN
F 3 "~" H 4350 4350 60  0000 C CNN
	1    4350 4350
	0    -1   -1   0   
$EndComp
$Comp
L C C11
U 1 1 5208A07F
P 4350 4150
F 0 "C11" H 4350 4250 40  0000 L CNN
F 1 "47n" H 4356 4065 40  0000 L CNN
F 2 "~" H 4388 4000 30  0000 C CNN
F 3 "~" H 4350 4150 60  0000 C CNN
	1    4350 4150
	0    -1   -1   0   
$EndComp
$Comp
L C C10
U 1 1 5208A085
P 4350 3950
F 0 "C10" H 4350 4050 40  0000 L CNN
F 1 "100n" H 4356 3865 40  0000 L CNN
F 2 "~" H 4388 3800 30  0000 C CNN
F 3 "~" H 4350 3950 60  0000 C CNN
	1    4350 3950
	0    -1   -1   0   
$EndComp
$Comp
L C C9
U 1 1 5208A08B
P 4350 3750
F 0 "C9" H 4350 3850 40  0000 L CNN
F 1 "100n" H 4356 3665 40  0000 L CNN
F 2 "~" H 4388 3600 30  0000 C CNN
F 3 "~" H 4350 3750 60  0000 C CNN
	1    4350 3750
	0    -1   -1   0   
$EndComp
$Comp
L C C8
U 1 1 5208A091
P 4350 3550
F 0 "C8" H 4350 3650 40  0000 L CNN
F 1 "100n" H 4356 3465 40  0000 L CNN
F 2 "~" H 4388 3400 30  0000 C CNN
F 3 "~" H 4350 3550 60  0000 C CNN
	1    4350 3550
	0    -1   -1   0   
$EndComp
$Comp
L C C7
U 1 1 5208A097
P 4350 3350
F 0 "C7" H 4350 3450 40  0000 L CNN
F 1 "47n" H 4356 3265 40  0000 L CNN
F 2 "~" H 4388 3200 30  0000 C CNN
F 3 "~" H 4350 3350 60  0000 C CNN
	1    4350 3350
	0    -1   -1   0   
$EndComp
$Comp
L C C6
U 1 1 5208A09D
P 4350 3150
F 0 "C6" H 4350 3250 40  0000 L CNN
F 1 "100n" H 4356 3065 40  0000 L CNN
F 2 "~" H 4388 3000 30  0000 C CNN
F 3 "~" H 4350 3150 60  0000 C CNN
	1    4350 3150
	0    -1   -1   0   
$EndComp
$Comp
L C C5
U 1 1 5208A0A3
P 4350 2950
F 0 "C5" H 4350 3050 40  0000 L CNN
F 1 "100n" H 4356 2865 40  0000 L CNN
F 2 "~" H 4388 2800 30  0000 C CNN
F 3 "~" H 4350 2950 60  0000 C CNN
	1    4350 2950
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR01
U 1 1 5208A0A9
P 4600 4400
F 0 "#PWR01" H 4600 4400 30  0001 C CNN
F 1 "GND" H 4600 4330 30  0001 C CNN
F 2 "" H 4600 4400 60  0000 C CNN
F 3 "" H 4600 4400 60  0000 C CNN
	1    4600 4400
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR02
U 1 1 5208A0B5
P 2750 3050
F 0 "#PWR02" H 2750 3050 30  0001 C CNN
F 1 "GND" H 2750 2980 30  0001 C CNN
F 2 "" H 2750 3050 60  0000 C CNN
F 3 "" H 2750 3050 60  0000 C CNN
	1    2750 3050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR03
U 1 1 5208A0BB
P 2750 5950
F 0 "#PWR03" H 2750 5950 30  0001 C CNN
F 1 "GND" H 2750 5880 30  0001 C CNN
F 2 "" H 2750 5950 60  0000 C CNN
F 3 "" H 2750 5950 60  0000 C CNN
	1    2750 5950
	1    0    0    -1  
$EndComp
$Comp
L TL074 U4
U 1 1 5208A0C7
P 5700 5300
F 0 "U4" H 5750 5500 60  0000 C CNN
F 1 "TL074" H 5850 5100 50  0000 C CNN
F 2 "" H 5700 5300 60  0000 C CNN
F 3 "" H 5700 5300 60  0000 C CNN
	1    5700 5300
	1    0    0    -1  
$EndComp
$Comp
L TL074 U4
U 2 1 5208A0CD
P 5700 2500
F 0 "U4" H 5750 2700 60  0000 C CNN
F 1 "TL074" H 5850 2300 50  0000 C CNN
F 2 "" H 5700 2500 60  0000 C CNN
F 3 "" H 5700 2500 60  0000 C CNN
	2    5700 2500
	1    0    0    -1  
$EndComp
$Comp
L TL074 U4
U 3 1 5208A0D3
P 5700 3400
F 0 "U4" H 5750 3600 60  0000 C CNN
F 1 "TL074" H 5850 3200 50  0000 C CNN
F 2 "" H 5700 3400 60  0000 C CNN
F 3 "" H 5700 3400 60  0000 C CNN
	3    5700 3400
	1    0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 5208A0DD
P 4400 2400
F 0 "R2" V 4500 2400 60  0000 C CNN
F 1 "15k" V 4400 2400 60  0000 C CNN
F 2 "" H 4400 2400 60  0001 C CNN
F 3 "" H 4400 2400 60  0001 C CNN
F 4 "R" V 4480 2400 40  0001 C CNN "Reference"
F 5 "15k" V 4407 2401 40  0001 C CNN "Value"
F 6 "~" V 4330 2400 30  0000 C CNN "Footprint"
F 7 "~" H 4400 2400 30  0000 C CNN "Datasheet"
	1    4400 2400
	0    -1   -1   0   
$EndComp
$Comp
L R R1
U 1 1 5208A0E7
P 4400 2100
F 0 "R1" V 4500 2100 60  0000 C CNN
F 1 "15k" V 4400 2100 60  0000 C CNN
F 2 "" H 4400 2100 60  0001 C CNN
F 3 "" H 4400 2100 60  0001 C CNN
F 4 "R" V 4480 2100 40  0001 C CNN "Reference"
F 5 "15k" V 4407 2101 40  0001 C CNN "Value"
F 6 "~" V 4330 2100 30  0000 C CNN "Footprint"
F 7 "~" H 4400 2100 30  0000 C CNN "Datasheet"
	1    4400 2100
	0    -1   -1   0   
$EndComp
$Comp
L R R5
U 1 1 5208A0F1
P 4800 3000
F 0 "R5" V 4900 3000 60  0000 C CNN
F 1 "220" V 4800 3000 60  0000 C CNN
F 2 "" H 4800 3000 60  0001 C CNN
F 3 "" H 4800 3000 60  0001 C CNN
F 4 "~" V 4880 3000 40  0000 C CNN "Reference"
F 5 "~" V 4807 3001 40  0000 C CNN "Value"
F 6 "~" V 4730 3000 30  0000 C CNN "Footprint"
F 7 "~" H 4800 3000 30  0000 C CNN "Datasheet"
	1    4800 3000
	-1   0    0    1   
$EndComp
$Comp
L R R6
U 1 1 5208A0FB
P 4800 4300
F 0 "R6" V 4700 4300 60  0000 C CNN
F 1 "220" V 4800 4300 60  0000 C CNN
F 2 "" H 4800 4300 60  0001 C CNN
F 3 "" H 4800 4300 60  0001 C CNN
F 4 "~" V 4900 4300 40  0000 C CNN "Reference"
F 5 "~" V 4807 4301 40  0000 C CNN "Value"
F 6 "~" V 4730 4300 30  0000 C CNN "Footprint"
F 7 "~" H 4800 4300 30  0000 C CNN "Datasheet"
	1    4800 4300
	1    0    0    -1  
$EndComp
$Comp
L R R4
U 1 1 5208A105
P 4400 5600
F 0 "R4" V 4500 5600 60  0000 C CNN
F 1 "15k" V 4400 5600 60  0000 C CNN
F 2 "" H 4400 5600 60  0001 C CNN
F 3 "" H 4400 5600 60  0001 C CNN
F 4 "R" V 4480 5600 40  0001 C CNN "Reference"
F 5 "15k" V 4407 5601 40  0001 C CNN "Value"
F 6 "~" V 4330 5600 30  0000 C CNN "Footprint"
F 7 "~" H 4400 5600 30  0000 C CNN "Datasheet"
	1    4400 5600
	0    -1   -1   0   
$EndComp
$Comp
L R R3
U 1 1 5208A10F
P 4400 5300
F 0 "R3" V 4500 5300 60  0000 C CNN
F 1 "15k" V 4400 5300 60  0000 C CNN
F 2 "" H 4400 5300 60  0001 C CNN
F 3 "" H 4400 5300 60  0001 C CNN
	1    4400 5300
	0    -1   -1   0   
$EndComp
$Comp
L CP1 C3
U 1 1 5208A119
P 3900 5300
F 0 "C3" H 3950 5350 60  0000 C CNN
F 1 "10u" H 3900 5300 60  0000 C CNN
F 2 "" H 3900 5300 60  0001 C CNN
F 3 "" H 3900 5300 60  0001 C CNN
F 4 "C" H 3950 5400 50  0000 L CNN "Reference"
F 5 "10u" H 3950 5200 50  0000 L CNN "Value"
F 6 "~" H 3900 5300 60  0000 C CNN "Footprint"
F 7 "~" H 3900 5300 60  0000 C CNN "Datasheet"
	1    3900 5300
	0    -1   -1   0   
$EndComp
$Comp
L CP1 C4
U 1 1 5208A123
P 3900 5600
F 0 "C4" H 3950 5650 60  0000 C CNN
F 1 "10u" H 3900 5600 60  0000 C CNN
F 2 "" H 3900 5600 60  0001 C CNN
F 3 "" H 3900 5600 60  0001 C CNN
F 4 "C" H 3950 5700 50  0000 L CNN "Reference"
F 5 "10u" H 3950 5500 50  0000 L CNN "Value"
F 6 "~" H 3900 5600 60  0000 C CNN "Footprint"
F 7 "~" H 3900 5600 60  0000 C CNN "Datasheet"
	1    3900 5600
	0    -1   -1   0   
$EndComp
$Comp
L CP1 C1
U 1 1 5208A12D
P 3900 2100
F 0 "C1" H 3950 2150 60  0000 C CNN
F 1 "10u" H 3900 2100 60  0000 C CNN
F 2 "" H 3900 2100 60  0001 C CNN
F 3 "" H 3900 2100 60  0001 C CNN
F 4 "C" H 3950 2200 50  0000 L CNN "Reference"
F 5 "10u" H 3950 2000 50  0000 L CNN "Value"
F 6 "~" H 3900 2100 60  0000 C CNN "Footprint"
F 7 "~" H 3900 2100 60  0000 C CNN "Datasheet"
	1    3900 2100
	0    -1   -1   0   
$EndComp
$Comp
L CP1 C2
U 1 1 5208A137
P 3900 2400
F 0 "C2" H 3950 2450 60  0000 C CNN
F 1 "10u" H 3900 2400 60  0000 C CNN
F 2 "" H 3900 2400 60  0001 C CNN
F 3 "" H 3900 2400 60  0001 C CNN
F 4 "C" H 3950 2500 50  0000 L CNN "Reference"
F 5 "10u" H 3950 2300 50  0000 L CNN "Value"
F 6 "~" H 3900 2400 60  0000 C CNN "Footprint"
F 7 "~" H 3900 2400 60  0000 C CNN "Datasheet"
	1    3900 2400
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR04
U 1 1 5208A13D
P 6900 3550
F 0 "#PWR04" H 6900 3550 30  0001 C CNN
F 1 "GND" H 6900 3480 30  0001 C CNN
F 2 "" H 6900 3550 60  0000 C CNN
F 3 "" H 6900 3550 60  0000 C CNN
	1    6900 3550
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR05
U 1 1 5208A143
P 8500 2750
F 0 "#PWR05" H 8500 2840 20  0001 C CNN
F 1 "+5V" H 8500 2840 30  0000 C CNN
F 2 "" H 8500 2750 60  0000 C CNN
F 3 "" H 8500 2750 60  0000 C CNN
	1    8500 2750
	1    0    0    -1  
$EndComp
$Comp
L -5V #PWR06
U 1 1 5208A149
P 8500 3550
F 0 "#PWR06" H 8500 3690 20  0001 C CNN
F 1 "-5V" H 8500 3660 30  0000 C CNN
F 2 "" H 8500 3550 60  0000 C CNN
F 3 "" H 8500 3550 60  0000 C CNN
	1    8500 3550
	-1   0    0    1   
$EndComp
$Comp
L C C18
U 1 1 5208A14F
P 8800 3550
F 0 "C18" H 8800 3650 40  0000 L CNN
F 1 "10n" H 8806 3465 40  0000 L CNN
F 2 "~" H 8838 3400 30  0000 C CNN
F 3 "~" H 8800 3550 60  0000 C CNN
	1    8800 3550
	-1   0    0    1   
$EndComp
$Comp
L C C17
U 1 1 5208A155
P 8800 2850
F 0 "C17" H 8800 2950 40  0000 L CNN
F 1 "10n" H 8806 2765 40  0000 L CNN
F 2 "~" H 8838 2700 30  0000 C CNN
F 3 "~" H 8800 2850 60  0000 C CNN
	1    8800 2850
	-1   0    0    1   
$EndComp
$Comp
L C C14
U 1 1 5208A15B
P 6450 3550
F 0 "C14" H 6450 3650 40  0000 L CNN
F 1 "10n" H 6456 3465 40  0000 L CNN
F 2 "~" H 6488 3400 30  0000 C CNN
F 3 "~" H 6450 3550 60  0000 C CNN
	1    6450 3550
	1    0    0    -1  
$EndComp
$Comp
L C C13
U 1 1 5208A161
P 6450 2850
F 0 "C13" H 6450 2950 40  0000 L CNN
F 1 "820p" H 6456 2765 40  0000 L CNN
F 2 "~" H 6488 2700 30  0000 C CNN
F 3 "~" H 6450 2850 60  0000 C CNN
	1    6450 2850
	1    0    0    -1  
$EndComp
$Comp
L R R11
U 1 1 5208A167
P 9050 2850
F 0 "R11" V 9130 2850 40  0000 C CNN
F 1 "220" V 9057 2851 40  0000 C CNN
F 2 "~" V 8980 2850 30  0000 C CNN
F 3 "~" H 9050 2850 30  0000 C CNN
	1    9050 2850
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR07
U 1 1 5208A16D
P 9200 3200
F 0 "#PWR07" H 9200 3200 30  0001 C CNN
F 1 "GND" H 9200 3130 30  0001 C CNN
F 2 "" H 9200 3200 60  0000 C CNN
F 3 "" H 9200 3200 60  0000 C CNN
	1    9200 3200
	1    0    0    -1  
$EndComp
$Comp
L R R12
U 1 1 5208A173
P 9050 3450
F 0 "R12" V 9130 3450 40  0000 C CNN
F 1 "470" V 9057 3451 40  0000 C CNN
F 2 "~" V 8980 3450 30  0000 C CNN
F 3 "~" H 9050 3450 30  0000 C CNN
	1    9050 3450
	1    0    0    -1  
$EndComp
$Comp
L SSM2044 U6
U 1 1 5208A179
P 7700 4950
F 0 "U6" H 7700 4850 50  0000 C CNN
F 1 "SSM2044" H 7700 5050 50  0000 C CNN
F 2 "MODULE" H 7700 4950 50  0001 C CNN
F 3 "DOCUMENTATION" H 7700 4950 50  0001 C CNN
	1    7700 4950
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR08
U 1 1 5208A17F
P 6900 5350
F 0 "#PWR08" H 6900 5350 30  0001 C CNN
F 1 "GND" H 6900 5280 30  0001 C CNN
F 2 "" H 6900 5350 60  0000 C CNN
F 3 "" H 6900 5350 60  0000 C CNN
	1    6900 5350
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR09
U 1 1 5208A185
P 8500 4550
F 0 "#PWR09" H 8500 4640 20  0001 C CNN
F 1 "+5V" H 8500 4640 30  0000 C CNN
F 2 "" H 8500 4550 60  0000 C CNN
F 3 "" H 8500 4550 60  0000 C CNN
	1    8500 4550
	1    0    0    -1  
$EndComp
$Comp
L -5V #PWR010
U 1 1 5208A18B
P 8500 5350
F 0 "#PWR010" H 8500 5490 20  0001 C CNN
F 1 "-5V" H 8500 5460 30  0000 C CNN
F 2 "" H 8500 5350 60  0000 C CNN
F 3 "" H 8500 5350 60  0000 C CNN
	1    8500 5350
	-1   0    0    1   
$EndComp
$Comp
L C C20
U 1 1 5208A191
P 8800 5350
F 0 "C20" H 8800 5450 40  0000 L CNN
F 1 "10n" H 8806 5265 40  0000 L CNN
F 2 "~" H 8838 5200 30  0000 C CNN
F 3 "~" H 8800 5350 60  0000 C CNN
	1    8800 5350
	-1   0    0    1   
$EndComp
$Comp
L C C19
U 1 1 5208A197
P 8800 4650
F 0 "C19" H 8800 4750 40  0000 L CNN
F 1 "10n" H 8806 4565 40  0000 L CNN
F 2 "~" H 8838 4500 30  0000 C CNN
F 3 "~" H 8800 4650 60  0000 C CNN
	1    8800 4650
	-1   0    0    1   
$EndComp
$Comp
L C C16
U 1 1 5208A19D
P 6450 5350
F 0 "C16" H 6450 5450 40  0000 L CNN
F 1 "10n" H 6456 5265 40  0000 L CNN
F 2 "~" H 6488 5200 30  0000 C CNN
F 3 "~" H 6450 5350 60  0000 C CNN
	1    6450 5350
	1    0    0    -1  
$EndComp
$Comp
L C C15
U 1 1 5208A1A3
P 6450 4650
F 0 "C15" H 6450 4750 40  0000 L CNN
F 1 "820p" H 6456 4565 40  0000 L CNN
F 2 "~" H 6488 4500 30  0000 C CNN
F 3 "~" H 6450 4650 60  0000 C CNN
	1    6450 4650
	1    0    0    -1  
$EndComp
$Comp
L R R7
U 1 1 5208A1A9
P 6550 2300
F 0 "R7" V 6630 2300 40  0000 C CNN
F 1 "4k7" V 6557 2301 40  0000 C CNN
F 2 "~" V 6480 2300 30  0000 C CNN
F 3 "~" H 6550 2300 30  0000 C CNN
	1    6550 2300
	0    -1   -1   0   
$EndComp
$Comp
L R R8
U 1 1 5208A1AF
P 6550 4200
F 0 "R8" V 6630 4200 40  0000 C CNN
F 1 "4k7" V 6557 4201 40  0000 C CNN
F 2 "~" V 6480 4200 30  0000 C CNN
F 3 "~" H 6550 4200 30  0000 C CNN
	1    6550 4200
	0    -1   -1   0   
$EndComp
$Comp
L R R9
U 1 1 5208A2A2
P 7700 3850
F 0 "R9" V 7780 3850 40  0000 C CNN
F 1 "6k8" V 7707 3851 40  0000 C CNN
F 2 "~" V 7630 3850 30  0000 C CNN
F 3 "~" H 7700 3850 30  0000 C CNN
	1    7700 3850
	0    -1   -1   0   
$EndComp
$Comp
L R R10
U 1 1 5208A2A8
P 7700 5650
F 0 "R10" V 7780 5650 40  0000 C CNN
F 1 "6k8" V 7707 5651 40  0000 C CNN
F 2 "~" V 7630 5650 30  0000 C CNN
F 3 "~" H 7700 5650 30  0000 C CNN
	1    7700 5650
	0    -1   -1   0   
$EndComp
$Comp
L R R17
U 1 1 5208A2BA
P 9400 3750
F 0 "R17" V 9480 3750 40  0000 C CNN
F 1 "18k" V 9407 3751 40  0000 C CNN
F 2 "~" V 9330 3750 30  0000 C CNN
F 3 "~" H 9400 3750 30  0000 C CNN
	1    9400 3750
	0    1    1    0   
$EndComp
$Comp
L -5V #PWR011
U 1 1 5208A2C0
P 9700 3800
F 0 "#PWR011" H 9700 3940 20  0001 C CNN
F 1 "-5V" H 9700 3910 30  0000 C CNN
F 2 "" H 9700 3800 60  0000 C CNN
F 3 "" H 9700 3800 60  0000 C CNN
	1    9700 3800
	-1   0    0    1   
$EndComp
$Comp
L CP1 C21
U 1 1 5208A2C8
P 9650 2500
F 0 "C21" H 9700 2600 50  0000 L CNN
F 1 "10u" H 9700 2400 50  0000 L CNN
F 2 "~" H 9650 2500 60  0000 C CNN
F 3 "~" H 9650 2500 60  0000 C CNN
	1    9650 2500
	0    -1   -1   0   
$EndComp
$Comp
L R R15
U 1 1 5208A2D0
P 9350 2850
F 0 "R15" V 9430 2850 40  0000 C CNN
F 1 "3k3" V 9357 2851 40  0000 C CNN
F 2 "~" V 9280 2850 30  0000 C CNN
F 3 "~" H 9350 2850 30  0000 C CNN
	1    9350 2850
	1    0    0    -1  
$EndComp
$Comp
L R R13
U 1 1 5208A2DA
P 9050 4650
F 0 "R13" V 9130 4650 40  0000 C CNN
F 1 "220" V 9057 4651 40  0000 C CNN
F 2 "~" V 8980 4650 30  0000 C CNN
F 3 "~" H 9050 4650 30  0000 C CNN
	1    9050 4650
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR012
U 1 1 5208A2E0
P 9200 5000
F 0 "#PWR012" H 9200 5000 30  0001 C CNN
F 1 "GND" H 9200 4930 30  0001 C CNN
F 2 "" H 9200 5000 60  0000 C CNN
F 3 "" H 9200 5000 60  0000 C CNN
	1    9200 5000
	1    0    0    -1  
$EndComp
$Comp
L R R14
U 1 1 5208A2E6
P 9050 5250
F 0 "R14" V 9130 5250 40  0000 C CNN
F 1 "470" V 9057 5251 40  0000 C CNN
F 2 "~" V 8980 5250 30  0000 C CNN
F 3 "~" H 9050 5250 30  0000 C CNN
	1    9050 5250
	1    0    0    -1  
$EndComp
$Comp
L R R18
U 1 1 5208A2F5
P 9400 5550
F 0 "R18" V 9480 5550 40  0000 C CNN
F 1 "18k" V 9407 5551 40  0000 C CNN
F 2 "~" V 9330 5550 30  0000 C CNN
F 3 "~" H 9400 5550 30  0000 C CNN
	1    9400 5550
	0    1    1    0   
$EndComp
$Comp
L -5V #PWR013
U 1 1 5208A2FB
P 9700 5600
F 0 "#PWR013" H 9700 5740 20  0001 C CNN
F 1 "-5V" H 9700 5710 30  0000 C CNN
F 2 "" H 9700 5600 60  0000 C CNN
F 3 "" H 9700 5600 60  0000 C CNN
	1    9700 5600
	-1   0    0    1   
$EndComp
$Comp
L CP1 C22
U 1 1 5208A302
P 9650 4300
F 0 "C22" H 9700 4400 50  0000 L CNN
F 1 "10u" H 9700 4200 50  0000 L CNN
F 2 "~" H 9650 4300 60  0000 C CNN
F 3 "~" H 9650 4300 60  0000 C CNN
	1    9650 4300
	0    -1   -1   0   
$EndComp
$Comp
L R R16
U 1 1 5208A30A
P 9350 4650
F 0 "R16" V 9430 4650 40  0000 C CNN
F 1 "3k3" V 9357 4651 40  0000 C CNN
F 2 "~" V 9280 4650 30  0000 C CNN
F 3 "~" H 9350 4650 30  0000 C CNN
	1    9350 4650
	1    0    0    -1  
$EndComp
$Comp
L -5V #PWR014
U 1 1 5208A314
P 5600 5800
F 0 "#PWR014" H 5600 5940 20  0001 C CNN
F 1 "-5V" H 5600 5910 30  0000 C CNN
F 2 "" H 5600 5800 60  0000 C CNN
F 3 "" H 5600 5800 60  0000 C CNN
	1    5600 5800
	-1   0    0    1   
$EndComp
$Comp
L +5V #PWR015
U 1 1 5208A31B
P 5600 2000
F 0 "#PWR015" H 5600 2090 20  0001 C CNN
F 1 "+5V" H 5600 2090 30  0000 C CNN
F 2 "" H 5600 2000 60  0000 C CNN
F 3 "" H 5600 2000 60  0000 C CNN
	1    5600 2000
	1    0    0    -1  
$EndComp
$Comp
L -5V #PWR016
U 1 1 5208A325
P 1900 4450
F 0 "#PWR016" H 1900 4590 20  0001 C CNN
F 1 "-5V" H 1900 4560 30  0000 C CNN
F 2 "" H 1900 4450 60  0000 C CNN
F 3 "" H 1900 4450 60  0000 C CNN
	1    1900 4450
	-1   0    0    1   
$EndComp
$Comp
L +5V #PWR017
U 1 1 5208A345
P 2750 2000
F 0 "#PWR017" H 2750 2090 20  0001 C CNN
F 1 "+5V" H 2750 2090 30  0000 C CNN
F 2 "" H 2750 2000 60  0000 C CNN
F 3 "" H 2750 2000 60  0000 C CNN
	1    2750 2000
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR018
U 1 1 5208A354
P 2750 4900
F 0 "#PWR018" H 2750 4990 20  0001 C CNN
F 1 "+5V" H 2750 4990 30  0000 C CNN
F 2 "" H 2750 4900 60  0000 C CNN
F 3 "" H 2750 4900 60  0000 C CNN
	1    2750 4900
	1    0    0    -1  
$EndComp
Text HLabel 9950 2500 2    60   Output ~ 0
OUT_A
Text HLabel 9950 4300 2    60   Output ~ 0
OUT_B
Text HLabel 900  1100 0    60   Input ~ 0
+5V
Text HLabel 900  1500 0    60   Input ~ 0
-5V
Text HLabel 900  1300 0    60   Input ~ 0
GND
Text HLabel 1400 2350 0    60   Input ~ 0
SDI
Text HLabel 1400 2450 0    60   Input ~ 0
SCK
Text HLabel 1400 2550 0    60   Input ~ 0
/CS
Text HLabel 1400 2650 0    60   Input ~ 0
/LDAC_A
Text HLabel 1400 5550 0    60   Input ~ 0
/LDAC_B
$Comp
L +5V #PWR019
U 1 1 5208A951
P 950 1050
F 0 "#PWR019" H 950 1140 20  0001 C CNN
F 1 "+5V" H 950 1140 30  0000 C CNN
F 2 "" H 950 1050 60  0000 C CNN
F 3 "" H 950 1050 60  0000 C CNN
	1    950  1050
	1    0    0    -1  
$EndComp
$Comp
L -5V #PWR020
U 1 1 5208A960
P 950 1550
F 0 "#PWR020" H 950 1690 20  0001 C CNN
F 1 "-5V" H 950 1660 30  0000 C CNN
F 2 "" H 950 1550 60  0000 C CNN
F 3 "" H 950 1550 60  0000 C CNN
	1    950  1550
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR021
U 1 1 5208A96F
P 950 1350
F 0 "#PWR021" H 950 1350 30  0001 C CNN
F 1 "GND" H 950 1280 30  0001 C CNN
F 2 "" H 950 1350 60  0000 C CNN
F 3 "" H 950 1350 60  0000 C CNN
	1    950  1350
	1    0    0    -1  
$EndComp
$Comp
L CONN_2 P2
U 1 1 5208B645
P 10300 3350
F 0 "P2" V 10250 3350 40  0000 C CNN
F 1 "OUT" V 10350 3350 40  0000 C CNN
F 2 "" H 10300 3350 60  0000 C CNN
F 3 "" H 10300 3350 60  0000 C CNN
	1    10300 3350
	1    0    0    -1  
$EndComp
$Comp
L CONN_3 K1
U 1 1 5208B654
P 1500 1300
F 0 "K1" V 1450 1300 50  0000 C CNN
F 1 "PWR" V 1550 1300 40  0000 C CNN
F 2 "" H 1500 1300 60  0000 C CNN
F 3 "" H 1500 1300 60  0000 C CNN
	1    1500 1300
	1    0    0    -1  
$EndComp
$Comp
L CONN_10 P1
U 1 1 5208BD9F
P 1050 3950
F 0 "P1" V 1000 3950 60  0000 C CNN
F 1 "DATA" V 1100 3950 60  0000 C CNN
F 2 "" H 1050 3950 60  0000 C CNN
F 3 "" H 1050 3950 60  0000 C CNN
	1    1050 3950
	-1   0    0    -1  
$EndComp
$Comp
L CP1 C23
U 1 1 5208A5A1
P 1900 900
F 0 "C23" H 1950 1000 50  0000 L CNN
F 1 "47u" H 1950 800 50  0000 L CNN
F 2 "~" H 1900 900 60  0000 C CNN
F 3 "~" H 1900 900 60  0000 C CNN
	1    1900 900 
	1    0    0    -1  
$EndComp
$Comp
L CP1 C24
U 1 1 5208A5B0
P 1900 1700
F 0 "C24" H 1950 1800 50  0000 L CNN
F 1 "47u" H 1950 1600 50  0000 L CNN
F 2 "~" H 1900 1700 60  0000 C CNN
F 3 "~" H 1900 1700 60  0000 C CNN
	1    1900 1700
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR022
U 1 1 5208A5BF
P 1800 1350
F 0 "#PWR022" H 1800 1350 30  0001 C CNN
F 1 "GND" H 1800 1280 30  0001 C CNN
F 2 "" H 1800 1350 60  0000 C CNN
F 3 "" H 1800 1350 60  0000 C CNN
	1    1800 1350
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR023
U 1 1 5208A5CE
P 1900 600
F 0 "#PWR023" H 1900 690 20  0001 C CNN
F 1 "+5V" H 1900 690 30  0000 C CNN
F 2 "" H 1900 600 60  0000 C CNN
F 3 "" H 1900 600 60  0000 C CNN
	1    1900 600 
	1    0    0    -1  
$EndComp
$Comp
L -5V #PWR024
U 1 1 5208A5EC
P 1900 2000
F 0 "#PWR024" H 1900 2140 20  0001 C CNN
F 1 "-5V" H 1900 2110 30  0000 C CNN
F 2 "" H 1900 2000 60  0000 C CNN
F 3 "" H 1900 2000 60  0000 C CNN
	1    1900 2000
	-1   0    0    1   
$EndComp
$Comp
L C C25
U 1 1 5208B48E
P 2200 900
F 0 "C25" H 2200 1000 40  0000 L CNN
F 1 "100n" H 2206 815 40  0000 L CNN
F 2 "~" H 2238 750 30  0000 C CNN
F 3 "~" H 2200 900 60  0000 C CNN
	1    2200 900 
	1    0    0    -1  
$EndComp
$Comp
L C C27
U 1 1 5208B49D
P 2400 900
F 0 "C27" H 2400 1000 40  0000 L CNN
F 1 "100n" H 2406 815 40  0000 L CNN
F 2 "~" H 2438 750 30  0000 C CNN
F 3 "~" H 2400 900 60  0000 C CNN
	1    2400 900 
	1    0    0    -1  
$EndComp
$Comp
L C C28
U 1 1 5208B4AC
P 2600 900
F 0 "C28" H 2600 1000 40  0000 L CNN
F 1 "100n" H 2606 815 40  0000 L CNN
F 2 "~" H 2638 750 30  0000 C CNN
F 3 "~" H 2600 900 60  0000 C CNN
	1    2600 900 
	1    0    0    -1  
$EndComp
$Comp
L C C26
U 1 1 5208B4BB
P 2200 1700
F 0 "C26" H 2200 1800 40  0000 L CNN
F 1 "100n" H 2206 1615 40  0000 L CNN
F 2 "~" H 2238 1550 30  0000 C CNN
F 3 "~" H 2200 1700 60  0000 C CNN
	1    2200 1700
	1    0    0    -1  
$EndComp
$Comp
L C C29
U 1 1 5208C610
P 2400 1700
F 0 "C29" H 2400 1800 40  0000 L CNN
F 1 "100n" H 2406 1615 40  0000 L CNN
F 2 "~" H 2438 1550 30  0000 C CNN
F 3 "~" H 2400 1700 60  0000 C CNN
	1    2400 1700
	1    0    0    -1  
$EndComp
$Comp
L VSS #PWR025
U 1 1 52091A24
P 2600 1350
F 0 "#PWR025" H 2600 1350 30  0001 C CNN
F 1 "VSS" H 2600 1280 30  0000 C CNN
F 2 "" H 2600 1350 60  0000 C CNN
F 3 "" H 2600 1350 60  0000 C CNN
	1    2600 1350
	1    0    0    -1  
$EndComp
$Comp
L VDD #PWR026
U 1 1 52092570
P 2600 600
F 0 "#PWR026" H 2600 700 30  0001 C CNN
F 1 "VDD" H 2600 710 30  0000 C CNN
F 2 "" H 2600 600 60  0000 C CNN
F 3 "" H 2600 600 60  0000 C CNN
	1    2600 600 
	1    0    0    -1  
$EndComp
Wire Wire Line
	1900 4450 1900 4400
Wire Wire Line
	1900 4400 1950 4400
Wire Wire Line
	4550 2950 4600 2950
Wire Wire Line
	4600 2950 4600 4400
Wire Wire Line
	4550 4350 4600 4350
Connection ~ 4600 4350
Wire Wire Line
	4550 4150 4600 4150
Connection ~ 4600 4150
Wire Wire Line
	4550 3950 4600 3950
Connection ~ 4600 3950
Wire Wire Line
	4600 3750 4550 3750
Connection ~ 4600 3750
Wire Wire Line
	4550 3550 4600 3550
Connection ~ 4600 3550
Wire Wire Line
	4550 3350 4600 3350
Connection ~ 4600 3350
Wire Wire Line
	4550 3150 4600 3150
Connection ~ 4600 3150
Wire Wire Line
	3350 4000 4000 4000
Wire Wire Line
	4000 4000 4000 4350
Wire Wire Line
	4000 4350 4150 4350
Wire Wire Line
	3350 3900 4050 3900
Wire Wire Line
	4050 3900 4050 4150
Wire Wire Line
	4050 4150 4150 4150
Wire Wire Line
	3350 3800 4100 3800
Wire Wire Line
	4100 3800 4100 3950
Wire Wire Line
	4100 3950 4150 3950
Wire Wire Line
	3350 3700 4100 3700
Wire Wire Line
	4100 3700 4100 3750
Wire Wire Line
	4100 3750 4150 3750
Wire Wire Line
	3350 3600 4100 3600
Wire Wire Line
	4100 3600 4100 3550
Wire Wire Line
	4100 3550 4150 3550
Wire Wire Line
	3350 3500 4100 3500
Wire Wire Line
	4100 3500 4100 3350
Wire Wire Line
	4100 3350 4150 3350
Wire Wire Line
	3350 3400 4050 3400
Wire Wire Line
	4050 3400 4050 3150
Wire Wire Line
	4050 3150 4150 3150
Wire Wire Line
	3350 3300 4000 3300
Wire Wire Line
	2750 3050 2750 3000
Wire Wire Line
	2750 2000 2750 2100
Wire Wire Line
	2750 5950 2750 5900
Wire Wire Line
	2750 4900 2750 5000
Wire Wire Line
	1750 2450 1750 5350
Wire Wire Line
	2250 4950 2750 4950
Connection ~ 2750 4950
Wire Wire Line
	2250 4950 2250 5650
Wire Wire Line
	2250 5650 2300 5650
Wire Wire Line
	1800 5250 2300 5250
Wire Wire Line
	1800 2350 1800 5250
Wire Wire Line
	1400 2350 2300 2350
Wire Wire Line
	1400 2450 2300 2450
Wire Wire Line
	1750 5350 2300 5350
Wire Wire Line
	1700 5450 2300 5450
Wire Wire Line
	1400 2550 2300 2550
Wire Wire Line
	2300 2750 2250 2750
Wire Wire Line
	2250 2750 2250 2050
Wire Wire Line
	2250 2050 2750 2050
Connection ~ 2750 2050
Wire Wire Line
	2900 2100 3450 2100
Wire Wire Line
	3650 2950 3650 3300
Wire Wire Line
	2900 3000 2900 3050
Wire Wire Line
	2900 3050 3550 3050
Wire Wire Line
	2900 5000 2900 4950
Wire Wire Line
	2900 4950 3600 4950
Wire Wire Line
	2900 5900 3650 5900
Wire Wire Line
	3650 5900 3650 5050
Wire Wire Line
	5200 4500 5150 4500
Wire Wire Line
	5150 4500 5150 4850
Wire Wire Line
	5150 4850 6250 4850
Wire Wire Line
	6250 4850 6250 4200
Wire Wire Line
	6250 4400 6200 4400
Wire Wire Line
	5200 5400 5150 5400
Wire Wire Line
	5150 5400 5150 5750
Wire Wire Line
	5150 5750 6250 5750
Wire Wire Line
	6250 5750 6250 5300
Wire Wire Line
	6250 5300 6200 5300
Wire Wire Line
	5200 3500 5150 3500
Wire Wire Line
	5150 3500 5150 3850
Wire Wire Line
	5150 3850 7450 3850
Wire Wire Line
	6250 3850 6250 3400
Wire Wire Line
	6250 3400 6200 3400
Wire Wire Line
	3400 5300 3700 5300
Wire Wire Line
	3400 5600 3700 5600
Wire Wire Line
	4100 5600 4150 5600
Wire Wire Line
	4100 5300 4150 5300
Wire Wire Line
	4100 2100 4150 2100
Wire Wire Line
	4100 2400 4150 2400
Wire Wire Line
	4800 2050 4800 2750
Wire Wire Line
	4800 2400 4650 2400
Wire Wire Line
	4800 2100 4650 2100
Connection ~ 4800 2400
Wire Wire Line
	4800 5600 4650 5600
Wire Wire Line
	4800 4550 4800 5600
Wire Wire Line
	4650 5300 4800 5300
Connection ~ 4800 5300
Wire Wire Line
	4800 3250 4800 4050
Wire Wire Line
	4800 3650 4600 3650
Connection ~ 4600 3650
Connection ~ 4800 3650
Wire Wire Line
	6900 3550 6900 3500
Wire Wire Line
	6900 3500 6950 3500
Wire Wire Line
	8450 3500 8500 3500
Wire Wire Line
	8500 3500 8500 3550
Wire Wire Line
	8450 2800 8500 2800
Wire Wire Line
	8500 2800 8500 2750
Wire Wire Line
	6450 3050 6450 3200
Wire Wire Line
	6450 3200 6950 3200
Wire Wire Line
	6450 3350 6450 3300
Wire Wire Line
	6450 3300 6950 3300
Wire Wire Line
	6450 3750 6450 3800
Wire Wire Line
	6450 3800 6600 3800
Wire Wire Line
	6600 3800 6600 3400
Wire Wire Line
	6600 3400 6950 3400
Wire Wire Line
	6600 2600 6600 3100
Wire Wire Line
	6600 3100 6950 3100
Wire Wire Line
	8650 2600 8800 2600
Wire Wire Line
	8800 2600 8800 2650
Wire Wire Line
	8450 3200 8800 3200
Wire Wire Line
	8800 3200 8800 3050
Wire Wire Line
	8450 3300 8800 3300
Wire Wire Line
	8800 3300 8800 3350
Wire Wire Line
	8650 3750 8800 3750
Wire Wire Line
	8650 3400 8450 3400
Wire Wire Line
	8650 3400 8650 3750
Wire Wire Line
	8650 2600 8650 3000
Wire Wire Line
	8450 2900 8600 2900
Wire Wire Line
	8600 2900 8600 2550
Wire Wire Line
	8600 2550 9050 2550
Wire Wire Line
	9050 2550 9050 2600
Wire Wire Line
	9200 3150 9200 3200
Wire Wire Line
	9050 3100 9050 3200
Wire Wire Line
	9050 3150 9350 3150
Connection ~ 9050 3150
Wire Wire Line
	8450 3100 8950 3100
Wire Wire Line
	8950 3100 8950 3850
Wire Wire Line
	9050 3700 9050 3750
Wire Wire Line
	8650 3000 8450 3000
Wire Wire Line
	6600 2600 6450 2600
Wire Wire Line
	6450 2600 6450 2650
Wire Wire Line
	3450 2100 3450 2950
Wire Wire Line
	3400 2400 3600 2400
Wire Wire Line
	3600 2400 3600 2100
Wire Wire Line
	3600 2100 3700 2100
Wire Wire Line
	3400 2700 3650 2700
Wire Wire Line
	3650 2700 3650 2400
Wire Wire Line
	3650 2400 3700 2400
Wire Wire Line
	3900 2700 3900 3500
Wire Wire Line
	3900 2700 5050 2700
Wire Wire Line
	5050 2700 5050 3300
Wire Wire Line
	5050 3300 5200 3300
Wire Wire Line
	3800 2550 3800 3600
Wire Wire Line
	3800 2550 5050 2550
Wire Wire Line
	5050 2550 5050 2400
Wire Wire Line
	5050 2400 5200 2400
Wire Wire Line
	3900 4000 3900 4800
Connection ~ 3900 4000
Wire Wire Line
	3800 3900 3800 4900
Connection ~ 3800 3900
Connection ~ 3900 3500
Connection ~ 3650 3300
Wire Wire Line
	4000 2950 4150 2950
Wire Wire Line
	4000 3300 4000 2950
Connection ~ 3800 3600
Wire Wire Line
	3550 3050 3550 3400
Connection ~ 3550 3400
Wire Wire Line
	3450 2950 3650 2950
Wire Wire Line
	6900 5350 6900 5300
Wire Wire Line
	6900 5300 6950 5300
Wire Wire Line
	8450 5300 8500 5300
Wire Wire Line
	8500 5300 8500 5350
Wire Wire Line
	8450 4600 8500 4600
Wire Wire Line
	8500 4600 8500 4550
Wire Wire Line
	6450 4850 6450 5000
Wire Wire Line
	6450 5000 6950 5000
Wire Wire Line
	6450 5150 6450 5100
Wire Wire Line
	6450 5100 6950 5100
Wire Wire Line
	6450 5550 6450 5600
Wire Wire Line
	6450 5600 6600 5600
Wire Wire Line
	6600 5600 6600 5200
Wire Wire Line
	6600 5200 6950 5200
Wire Wire Line
	6600 4400 6600 4900
Wire Wire Line
	6600 4900 6950 4900
Wire Wire Line
	8650 4400 8800 4400
Wire Wire Line
	8800 4400 8800 4450
Wire Wire Line
	8450 5000 8800 5000
Wire Wire Line
	8800 5000 8800 4850
Wire Wire Line
	8450 5100 8800 5100
Wire Wire Line
	8800 5100 8800 5150
Wire Wire Line
	8650 5550 8800 5550
Wire Wire Line
	8650 5200 8450 5200
Wire Wire Line
	8650 5200 8650 5550
Wire Wire Line
	8650 4400 8650 4800
Wire Wire Line
	8450 4700 8600 4700
Wire Wire Line
	8600 4700 8600 4350
Wire Wire Line
	8450 4900 8950 4900
Wire Wire Line
	8950 4900 8950 5650
Wire Wire Line
	8650 4800 8450 4800
Wire Wire Line
	6600 4400 6450 4400
Wire Wire Line
	6450 4400 6450 4450
Wire Wire Line
	6900 2050 6900 2800
Wire Wire Line
	6900 2800 6950 2800
Connection ~ 4800 2100
Wire Wire Line
	6850 2300 6850 2900
Wire Wire Line
	6850 2900 6950 2900
Wire Wire Line
	3800 4900 5050 4900
Wire Wire Line
	5050 4900 5050 5200
Wire Wire Line
	5050 5200 5200 5200
Wire Wire Line
	3900 4800 5050 4800
Wire Wire Line
	5050 4800 5050 4300
Wire Wire Line
	5050 4300 5200 4300
Wire Wire Line
	4800 4700 4950 4700
Wire Wire Line
	4950 4700 4950 3950
Wire Wire Line
	4950 3950 6900 3950
Wire Wire Line
	6900 3950 6900 4600
Wire Wire Line
	6900 4600 6950 4600
Connection ~ 4800 4700
Wire Wire Line
	5200 2600 5150 2600
Wire Wire Line
	5150 2600 5150 2950
Wire Wire Line
	5150 2950 6250 2950
Connection ~ 6250 2500
Wire Wire Line
	6200 2500 6250 2500
Wire Wire Line
	6250 2950 6250 2300
Wire Wire Line
	6250 2300 6300 2300
Wire Wire Line
	6800 2300 6850 2300
Wire Wire Line
	6250 4200 6300 4200
Connection ~ 6250 4400
Wire Wire Line
	6800 4200 6850 4200
Wire Wire Line
	6850 4200 6850 4700
Wire Wire Line
	6850 4700 6950 4700
Wire Wire Line
	6700 2500 9450 2500
Wire Wire Line
	8950 3850 7950 3850
Connection ~ 9050 3750
Wire Wire Line
	8950 5650 7950 5650
Connection ~ 6250 3850
Wire Wire Line
	7450 5650 6250 5650
Connection ~ 6250 5650
Wire Wire Line
	3700 3800 3700 5050
Connection ~ 3700 3800
Wire Wire Line
	3700 5050 3650 5050
Wire Wire Line
	3600 4950 3600 3700
Connection ~ 3600 3700
Connection ~ 8950 5550
Wire Wire Line
	9700 3750 9700 3800
Connection ~ 8950 3750
Wire Wire Line
	8950 3750 9150 3750
Wire Wire Line
	9650 3750 9700 3750
Wire Wire Line
	9350 3150 9350 3100
Connection ~ 9200 3150
Wire Wire Line
	9350 2500 9350 2600
Wire Wire Line
	8600 4350 9050 4350
Wire Wire Line
	9050 4350 9050 4400
Wire Wire Line
	9200 4950 9200 5000
Wire Wire Line
	9050 4900 9050 5000
Wire Wire Line
	9050 4950 9350 4950
Connection ~ 9050 4950
Wire Wire Line
	9050 5500 9050 5550
Connection ~ 9050 5550
Wire Wire Line
	9700 5550 9700 5600
Wire Wire Line
	8950 5550 9150 5550
Wire Wire Line
	9650 5550 9700 5550
Wire Wire Line
	9350 4950 9350 4900
Connection ~ 9200 4950
Wire Wire Line
	9350 4300 9350 4400
Wire Wire Line
	4800 2050 6900 2050
Wire Wire Line
	5600 2100 5600 2000
Wire Wire Line
	5600 5800 5600 5700
Wire Wire Line
	9850 2500 9950 2500
Wire Wire Line
	900  1500 1100 1500
Wire Wire Line
	950  1500 950  1550
Wire Wire Line
	900  1300 1150 1300
Wire Wire Line
	950  1300 950  1350
Wire Wire Line
	900  1100 1100 1100
Wire Wire Line
	950  1100 950  1050
Connection ~ 1750 2450
Wire Wire Line
	1400 2650 2300 2650
Wire Wire Line
	1400 5550 2300 5550
Wire Wire Line
	1650 2650 1650 4400
Connection ~ 1650 2650
Wire Wire Line
	3350 4300 3450 4300
Wire Wire Line
	3450 4300 3450 4700
Wire Wire Line
	3450 4700 1550 4700
Wire Wire Line
	1550 4700 1550 3700
Wire Wire Line
	1850 3300 1950 3300
Connection ~ 950  1300
Wire Wire Line
	1100 1500 1100 1400
Wire Wire Line
	1100 1400 1150 1400
Connection ~ 950  1500
Wire Wire Line
	1150 1200 1100 1200
Wire Wire Line
	1100 1200 1100 1100
Connection ~ 950  1100
Wire Wire Line
	9850 4300 9950 4300
Wire Wire Line
	9900 4300 9900 3450
Wire Wire Line
	9900 3450 9950 3450
Connection ~ 9900 4300
Wire Wire Line
	9950 3250 9900 3250
Wire Wire Line
	9900 3250 9900 2500
Connection ~ 9900 2500
Wire Wire Line
	1900 1900 1900 2000
Wire Wire Line
	1900 600  1900 700 
Wire Wire Line
	1900 1100 1900 1500
Wire Wire Line
	1800 1300 2600 1300
Connection ~ 1900 1300
Wire Wire Line
	2200 1950 2200 1900
Wire Wire Line
	1900 1950 2400 1950
Connection ~ 1900 1950
Wire Wire Line
	2200 1100 2200 1500
Connection ~ 2200 1300
Wire Wire Line
	1800 1350 1800 1300
Wire Wire Line
	2600 1100 2600 1350
Wire Wire Line
	2400 1100 2400 1500
Connection ~ 2400 1300
Wire Wire Line
	1900 650  2600 650 
Wire Wire Line
	2600 600  2600 700 
Connection ~ 1900 650 
Wire Wire Line
	2400 700  2400 650 
Connection ~ 2400 650 
Wire Wire Line
	2200 700  2200 650 
Connection ~ 2200 650 
Wire Wire Line
	2400 1950 2400 1900
Connection ~ 2200 1950
Wire Wire Line
	6700 4300 9450 4300
Wire Wire Line
	6950 4800 6700 4800
Wire Wire Line
	6700 4800 6700 4300
Connection ~ 9350 4300
Wire Wire Line
	6950 3000 6700 3000
Wire Wire Line
	6700 3000 6700 2500
Connection ~ 9350 2500
Connection ~ 1800 2350
Wire Wire Line
	1700 2550 1700 5450
Wire Wire Line
	3350 4500 3550 4500
Wire Wire Line
	3550 4500 3550 4800
Wire Wire Line
	3550 4800 1450 4800
Wire Wire Line
	1450 4800 1450 3500
Wire Wire Line
	3350 4200 3400 4200
Wire Wire Line
	3400 4200 3400 4650
Wire Wire Line
	3400 4650 1600 4650
Wire Wire Line
	1600 4650 1600 3900
Wire Wire Line
	3350 4400 3500 4400
Wire Wire Line
	3500 4400 3500 4750
Wire Wire Line
	3500 4750 1500 4750
Connection ~ 2600 650 
Connection ~ 2600 1300
Connection ~ 1700 2550
Wire Wire Line
	1500 4750 1500 3600
Wire Wire Line
	1450 3500 1400 3500
Wire Wire Line
	1500 3600 1400 3600
Wire Wire Line
	1550 3700 1400 3700
Wire Wire Line
	1850 3800 1850 3300
Wire Wire Line
	1400 4000 1700 4000
Connection ~ 1700 4000
Wire Wire Line
	1400 4100 1750 4100
Connection ~ 1750 4100
Wire Wire Line
	1400 4200 1800 4200
Connection ~ 1800 4200
Wire Wire Line
	1650 4400 1400 4400
Wire Wire Line
	1400 4300 1850 4300
Wire Wire Line
	1850 4300 1850 5550
Wire Wire Line
	1400 3800 1850 3800
Wire Wire Line
	1600 3900 1400 3900
Connection ~ 1850 5550
$EndSCHEMATC
