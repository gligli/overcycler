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
LIBS:conn_33
LIBS:optocoupler
LIBS:overcycler-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Overcycler main board schematics"
Date "22 aug 2013"
Rev "1"
Comp "GliGli's DIY"
Comment1 "inverser sck/ssel"
Comment2 "largeur lpc"
Comment3 "espacement ports cards"
Comment4 ""
$EndDescr
$Comp
L CONN_2 P11
U 1 1 51FF8E00
P 7400 1550
F 0 "P11" V 7350 1550 40  0000 C CNN
F 1 "AC_IN" V 7450 1550 40  0000 C CNN
F 2 "" H 7400 1550 60  0000 C CNN
F 3 "" H 7400 1550 60  0000 C CNN
	1    7400 1550
	-1   0    0    -1  
$EndComp
$Comp
L CP1 C4
U 1 1 51FF8EA9
P 8000 1200
F 0 "C4" H 8050 1300 50  0000 L CNN
F 1 "3300u" H 8050 1100 50  0000 L CNN
F 2 "~" H 8000 1200 60  0000 C CNN
F 3 "~" H 8000 1200 60  0000 C CNN
	1    8000 1200
	1    0    0    -1  
$EndComp
$Comp
L CP1 C5
U 1 1 51FF8EC7
P 8000 1900
F 0 "C5" H 8050 2000 50  0000 L CNN
F 1 "2200u" H 8050 1800 50  0000 L CNN
F 2 "~" H 8000 1900 60  0000 C CNN
F 3 "~" H 8000 1900 60  0000 C CNN
	1    8000 1900
	1    0    0    -1  
$EndComp
$Comp
L CP1 C6
U 1 1 51FF908C
P 8900 1200
F 0 "C6" H 8950 1300 50  0000 L CNN
F 1 "6u8" H 8950 1100 50  0000 L CNN
F 2 "~" H 8900 1200 60  0000 C CNN
F 3 "~" H 8900 1200 60  0000 C CNN
	1    8900 1200
	1    0    0    -1  
$EndComp
$Comp
L CP1 C7
U 1 1 51FF909B
P 8900 1900
F 0 "C7" H 8950 2000 50  0000 L CNN
F 1 "4u7" H 8950 1800 50  0000 L CNN
F 2 "~" H 8900 1900 60  0000 C CNN
F 3 "~" H 8900 1900 60  0000 C CNN
	1    8900 1900
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR01
U 1 1 51FF9119
P 7250 2600
F 0 "#PWR01" H 7250 2690 20  0001 C CNN
F 1 "+5V" H 7250 2690 30  0000 C CNN
F 2 "" H 7250 2600 60  0000 C CNN
F 3 "" H 7250 2600 60  0000 C CNN
	1    7250 2600
	1    0    0    -1  
$EndComp
$Comp
L -5V #PWR02
U 1 1 51FF9128
P 8900 2200
F 0 "#PWR02" H 8900 2340 20  0001 C CNN
F 1 "-5V" H 8900 2310 30  0000 C CNN
F 2 "" H 8900 2200 60  0000 C CNN
F 3 "" H 8900 2200 60  0000 C CNN
	1    8900 2200
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR03
U 1 1 51FF93D2
P 9050 1700
F 0 "#PWR03" H 9050 1700 30  0001 C CNN
F 1 "GND" H 9050 1630 30  0001 C CNN
F 2 "" H 9050 1700 60  0000 C CNN
F 3 "" H 9050 1700 60  0000 C CNN
	1    9050 1700
	1    0    0    -1  
$EndComp
$Comp
L DIODESCH D1
U 1 1 51FF9C04
P 7750 950
F 0 "D1" H 7750 1050 40  0000 C CNN
F 1 "1N5819" H 7750 850 40  0000 C CNN
F 2 "~" H 7750 950 60  0000 C CNN
F 3 "~" H 7750 950 60  0000 C CNN
	1    7750 950 
	1    0    0    -1  
$EndComp
$Comp
L DIODESCH D2
U 1 1 51FF9C13
P 7750 2150
F 0 "D2" H 7750 2250 40  0000 C CNN
F 1 "1N5819" H 7750 2050 40  0000 C CNN
F 2 "~" H 7750 2150 60  0000 C CNN
F 3 "~" H 7750 2150 60  0000 C CNN
	1    7750 2150
	-1   0    0    1   
$EndComp
$Comp
L LM1084IT-3.3/NOPB U6
U 1 1 5213E258
P 9700 1000
F 0 "U6" H 9900 800 40  0000 C CNN
F 1 "LM1084IT-3.3/NOPB" H 9400 1200 40  0000 L CNN
F 2 "TO-220" H 9700 1100 30  0000 C CIN
F 3 "~" H 9700 1000 60  0000 C CNN
	1    9700 1000
	1    0    0    -1  
$EndComp
$Comp
L LM1084IT-5.0/NOPB U4
U 1 1 5213E267
P 8450 1000
F 0 "U4" H 8650 800 40  0000 C CNN
F 1 "LM1084IT-5.0/NOPB" H 8150 1200 40  0000 L CNN
F 2 "TO-220" H 8450 1100 30  0000 C CIN
F 3 "~" H 8450 1000 60  0000 C CNN
	1    8450 1000
	1    0    0    -1  
$EndComp
$Comp
L LM7905CT U5
U 1 1 5213E276
P 8450 2100
F 0 "U5" H 8250 1900 40  0000 C CNN
F 1 "LM2990-5.0/NOPB" H 8450 1900 40  0000 L CNN
F 2 "TO-220" H 8450 2000 30  0000 C CIN
F 3 "~" H 8450 2100 60  0000 C CNN
	1    8450 2100
	1    0    0    -1  
$EndComp
$Comp
L CP1 C8
U 1 1 5213E28D
P 10150 1200
F 0 "C8" H 10200 1300 50  0000 L CNN
F 1 "6u8" H 10200 1100 50  0000 L CNN
F 2 "~" H 10150 1200 60  0000 C CNN
F 3 "~" H 10150 1200 60  0000 C CNN
	1    10150 1200
	1    0    0    -1  
$EndComp
$Comp
L +3.3V #PWR04
U 1 1 5213E328
P 10150 900
F 0 "#PWR04" H 10150 860 30  0001 C CNN
F 1 "+3.3V" H 10150 1010 30  0000 C CNN
F 2 "" H 10150 900 60  0000 C CNN
F 3 "" H 10150 900 60  0000 C CNN
	1    10150 900 
	1    0    0    -1  
$EndComp
$Comp
L CONN_33 P1
U 1 1 5213EA91
P 700 2300
F 0 "P1" V 651 2300 60  0000 C CNN
F 1 "LPC_P1" V 750 2300 60  0000 C CNN
F 2 "~" H 700 2300 60  0000 C CNN
F 3 "" H 700 2300 60  0000 C CNN
	1    700  2300
	-1   0    0    -1  
$EndComp
$Comp
L CONN_33 P2
U 1 1 5213EAA0
P 700 5700
F 0 "P2" V 651 5700 60  0000 C CNN
F 1 "LPC_P2" V 750 5700 60  0000 C CNN
F 2 "" H 700 5700 60  0000 C CNN
F 3 "" H 700 5700 60  0000 C CNN
	1    700  5700
	-1   0    0    -1  
$EndComp
$Comp
L +3.3V #PWR05
U 1 1 5213EAD2
P 1250 4200
F 0 "#PWR05" H 1250 4160 30  0001 C CNN
F 1 "+3.3V" H 1250 4310 30  0000 C CNN
F 2 "" H 1250 4200 60  0000 C CNN
F 3 "" H 1250 4200 60  0000 C CNN
	1    1250 4200
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR06
U 1 1 5213EADF
P 2650 1150
F 0 "#PWR06" H 2650 1150 30  0001 C CNN
F 1 "GND" H 2650 1080 30  0001 C CNN
F 2 "" H 2650 1150 60  0000 C CNN
F 3 "" H 2650 1150 60  0000 C CNN
	1    2650 1150
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR07
U 1 1 5213EAE5
P 1750 850
F 0 "#PWR07" H 1750 940 20  0001 C CNN
F 1 "+5V" H 1750 940 30  0000 C CNN
F 2 "" H 1750 850 60  0000 C CNN
F 3 "" H 1750 850 60  0000 C CNN
	1    1750 850 
	1    0    0    -1  
$EndComp
$Comp
L CONN_6 P3
U 1 1 5213EBAF
P 2250 650
F 0 "P3" V 2200 650 60  0000 C CNN
F 1 "FT232" V 2300 650 60  0000 C CNN
F 2 "" H 2250 650 60  0000 C CNN
F 3 "" H 2250 650 60  0000 C CNN
	1    2250 650 
	0    -1   -1   0   
$EndComp
$Comp
L CONN_7 P5
U 1 1 5213EBBE
P 3300 700
F 0 "P5" V 3270 700 60  0000 C CNN
F 1 "SDCARD" V 3370 700 60  0000 C CNN
F 2 "" H 3300 700 60  0000 C CNN
F 3 "" H 3300 700 60  0000 C CNN
	1    3300 700 
	0    -1   -1   0   
$EndComp
$Comp
L CONN_14 P4
U 1 1 5213EBCD
P 2300 7600
F 0 "P4" V 2270 7600 60  0000 C CNN
F 1 "CARD0_CMD_PWR" V 2380 7600 60  0000 C CNN
F 2 "" H 2300 7600 60  0000 C CNN
F 3 "" H 2300 7600 60  0000 C CNN
	1    2300 7600
	0    -1   1    0   
$EndComp
$Comp
L CONN_2 P13
U 1 1 5213EBFF
P 11000 6800
F 0 "P13" V 10950 6800 40  0000 C CNN
F 1 "MIDI_IN" V 11050 6800 40  0000 C CNN
F 2 "" H 11000 6800 60  0000 C CNN
F 3 "" H 11000 6800 60  0000 C CNN
	1    11000 6800
	1    0    0    -1  
$EndComp
$Comp
L SW_PUSH SW1
U 1 1 5213EEC8
P 1500 650
F 0 "SW1" H 1650 760 50  0000 C CNN
F 1 "RESET" H 1500 570 50  0000 C CNN
F 2 "~" H 1500 650 60  0000 C CNN
F 3 "~" H 1500 650 60  0000 C CNN
	1    1500 650 
	1    0    0    -1  
$EndComp
$Comp
L +3.3V #PWR08
U 1 1 5213F7CE
P 2650 850
F 0 "#PWR08" H 2650 810 30  0001 C CNN
F 1 "+3.3V" H 2650 960 30  0000 C CNN
F 2 "" H 2650 850 60  0000 C CNN
F 3 "" H 2650 850 60  0000 C CNN
	1    2650 850 
	1    0    0    -1  
$EndComp
$Comp
L CONN_5 P7
U 1 1 5213F932
P 4300 650
F 0 "P7" V 4250 650 50  0000 C CNN
F 1 "SCREEN" V 4350 650 50  0000 C CNN
F 2 "" H 4300 650 60  0000 C CNN
F 3 "" H 4300 650 60  0000 C CNN
	1    4300 650 
	0    -1   -1   0   
$EndComp
$Comp
L +5V #PWR09
U 1 1 5213F93F
P 4650 850
F 0 "#PWR09" H 4650 940 20  0001 C CNN
F 1 "+5V" H 4650 940 30  0000 C CNN
F 2 "" H 4650 850 60  0000 C CNN
F 3 "" H 4650 850 60  0000 C CNN
	1    4650 850 
	1    0    0    -1  
$EndComp
$Comp
L 4053 U1
U 1 1 5213FFB2
P 3650 2650
F 0 "U1" H 3750 2650 60  0000 C CNN
F 1 "4053" H 3750 2450 60  0000 C CNN
F 2 "" H 3650 2650 60  0000 C CNN
F 3 "" H 3650 2650 60  0000 C CNN
	1    3650 2650
	-1   0    0    -1  
$EndComp
$Comp
L 4053 U2
U 1 1 5213FFC4
P 3650 4400
F 0 "U2" H 3750 4400 60  0000 C CNN
F 1 "4053" H 3750 4200 60  0000 C CNN
F 2 "" H 3650 4400 60  0000 C CNN
F 3 "" H 3650 4400 60  0000 C CNN
	1    3650 4400
	-1   0    0    -1  
$EndComp
$Comp
L GND #PWR010
U 1 1 5213FFCA
P 3650 5150
F 0 "#PWR010" H 3650 5150 30  0001 C CNN
F 1 "GND" H 3650 5080 30  0001 C CNN
F 2 "" H 3650 5150 60  0000 C CNN
F 3 "" H 3650 5150 60  0000 C CNN
	1    3650 5150
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR011
U 1 1 5213FFD0
P 3650 3400
F 0 "#PWR011" H 3650 3400 30  0001 C CNN
F 1 "GND" H 3650 3330 30  0001 C CNN
F 2 "" H 3650 3400 60  0000 C CNN
F 3 "" H 3650 3400 60  0000 C CNN
	1    3650 3400
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR012
U 1 1 521401B5
P 3650 1900
F 0 "#PWR012" H 3650 1990 20  0001 C CNN
F 1 "+5V" H 3650 1990 30  0000 C CNN
F 2 "" H 3650 1900 60  0000 C CNN
F 3 "" H 3650 1900 60  0000 C CNN
	1    3650 1900
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR013
U 1 1 521401BB
P 3650 3650
F 0 "#PWR013" H 3650 3740 20  0001 C CNN
F 1 "+5V" H 3650 3740 30  0000 C CNN
F 2 "" H 3650 3650 60  0000 C CNN
F 3 "" H 3650 3650 60  0000 C CNN
	1    3650 3650
	1    0    0    -1  
$EndComp
$Comp
L +3.3V #PWR014
U 1 1 52142117
P 5050 1750
F 0 "#PWR014" H 5050 1710 30  0001 C CNN
F 1 "+3.3V" H 5050 1860 30  0000 C CNN
F 2 "" H 5050 1750 60  0000 C CNN
F 3 "" H 5050 1750 60  0000 C CNN
	1    5050 1750
	1    0    0    -1  
$EndComp
$Comp
L CONN_14 P6
U 1 1 521425A6
P 3800 7600
F 0 "P6" V 3770 7600 60  0000 C CNN
F 1 "CARD1_CMD_PWR" V 3880 7600 60  0000 C CNN
F 2 "" H 3800 7600 60  0000 C CNN
F 3 "" H 3800 7600 60  0000 C CNN
	1    3800 7600
	0    -1   1    0   
$EndComp
$Comp
L CONN_14 P8
U 1 1 521425AC
P 5300 7600
F 0 "P8" V 5270 7600 60  0000 C CNN
F 1 "CARD2_CMD_PWR" V 5380 7600 60  0000 C CNN
F 2 "" H 5300 7600 60  0000 C CNN
F 3 "" H 5300 7600 60  0000 C CNN
	1    5300 7600
	0    -1   1    0   
$EndComp
$Comp
L CONN_9 P10
U 1 1 5214394E
P 6550 7600
F 0 "P10" V 6500 7600 60  0000 C CNN
F 1 "VCA_CMD_PWR" V 6600 7600 60  0000 C CNN
F 2 "" H 6550 7600 60  0000 C CNN
F 3 "" H 6550 7600 60  0000 C CNN
	1    6550 7600
	0    -1   1    0   
$EndComp
$Comp
L 6N139 OC1
U 1 1 52151573
P 9450 6650
F 0 "OC1" H 9075 6975 50  0000 L BNN
F 1 "6N139" H 9075 6250 50  0000 L BNN
F 2 "optocoupler-DIL08" H 9450 6800 50  0001 C CNN
F 3 "" H 9450 6650 60  0000 C CNN
	1    9450 6650
	-1   0    0    -1  
$EndComp
$Comp
L DIODESCH D3
U 1 1 5215159F
P 10100 6650
F 0 "D3" H 10100 6750 40  0000 C CNN
F 1 "1N5819" H 10100 6550 40  0000 C CNN
F 2 "~" H 10100 6650 60  0000 C CNN
F 3 "~" H 10100 6650 60  0000 C CNN
	1    10100 6650
	0    -1   -1   0   
$EndComp
$Comp
L R R5
U 1 1 521515B8
P 10400 6400
F 0 "R5" V 10480 6400 40  0000 C CNN
F 1 "220" V 10407 6401 40  0000 C CNN
F 2 "~" V 10330 6400 30  0000 C CNN
F 3 "~" H 10400 6400 30  0000 C CNN
	1    10400 6400
	0    1    1    0   
$EndComp
$Comp
L R R2
U 1 1 52151B0A
P 8700 6450
F 0 "R2" V 8780 6450 40  0000 C CNN
F 1 "270" V 8707 6451 40  0000 C CNN
F 2 "~" V 8630 6450 30  0000 C CNN
F 3 "~" H 8700 6450 30  0000 C CNN
	1    8700 6450
	0    1    1    0   
$EndComp
$Comp
L R R3
U 1 1 52151B10
P 8700 6850
F 0 "R3" V 8780 6850 40  0000 C CNN
F 1 "4k7" V 8707 6851 40  0000 C CNN
F 2 "~" V 8630 6850 30  0000 C CNN
F 3 "~" H 8700 6850 30  0000 C CNN
	1    8700 6850
	0    1    1    0   
$EndComp
$Comp
L GND #PWR015
U 1 1 52151C47
P 9000 6900
F 0 "#PWR015" H 9000 6900 30  0001 C CNN
F 1 "GND" H 9000 6830 30  0001 C CNN
F 2 "" H 9000 6900 60  0000 C CNN
F 3 "" H 9000 6900 60  0000 C CNN
	1    9000 6900
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR016
U 1 1 52153743
P 7100 7000
F 0 "#PWR016" H 7100 7000 30  0001 C CNN
F 1 "GND" H 7100 6930 30  0001 C CNN
F 2 "" H 7100 7000 60  0000 C CNN
F 3 "" H 7100 7000 60  0000 C CNN
	1    7100 7000
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR017
U 1 1 52153749
P 6650 6900
F 0 "#PWR017" H 6650 6990 20  0001 C CNN
F 1 "+5V" H 6650 6990 30  0000 C CNN
F 2 "" H 6650 6900 60  0000 C CNN
F 3 "" H 6650 6900 60  0000 C CNN
	1    6650 6900
	1    0    0    -1  
$EndComp
$Comp
L -5V #PWR018
U 1 1 52153759
P 6750 6900
F 0 "#PWR018" H 6750 7040 20  0001 C CNN
F 1 "-5V" H 6750 7010 30  0000 C CNN
F 2 "" H 6750 6900 60  0000 C CNN
F 3 "" H 6750 6900 60  0000 C CNN
	1    6750 6900
	1    0    0    -1  
$EndComp
Text Label 8350 6450 2    60   ~ 0
MIDI_IN
Text Label 1050 6950 0    60   ~ 0
MIDI_IN
$Comp
L CONN_2 P12
U 1 1 5215A716
P 11000 5950
F 0 "P12" V 10950 5950 40  0000 C CNN
F 1 "FOOT_IN" V 11050 5950 40  0000 C CNN
F 2 "" H 11000 5950 60  0000 C CNN
F 3 "" H 11000 5950 60  0000 C CNN
	1    11000 5950
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR019
U 1 1 5215A723
P 10600 6100
F 0 "#PWR019" H 10600 6100 30  0001 C CNN
F 1 "GND" H 10600 6030 30  0001 C CNN
F 2 "" H 10600 6100 60  0000 C CNN
F 3 "" H 10600 6100 60  0000 C CNN
	1    10600 6100
	1    0    0    -1  
$EndComp
$Comp
L R R6
U 1 1 5215A9B3
P 10600 5550
F 0 "R6" V 10680 5550 40  0000 C CNN
F 1 "4k7" V 10607 5551 40  0000 C CNN
F 2 "~" V 10530 5550 30  0000 C CNN
F 3 "~" H 10600 5550 30  0000 C CNN
	1    10600 5550
	-1   0    0    1   
$EndComp
$Comp
L R R4
U 1 1 5215A9CF
P 10300 5850
F 0 "R4" V 10380 5850 40  0000 C CNN
F 1 "100k" V 10307 5851 40  0000 C CNN
F 2 "~" V 10230 5850 30  0000 C CNN
F 3 "~" H 10300 5850 30  0000 C CNN
	1    10300 5850
	0    -1   -1   0   
$EndComp
$Comp
L C C9
U 1 1 5215ABA1
P 10350 6050
F 0 "C9" H 10350 6150 40  0000 L CNN
F 1 "100n" H 10356 5965 40  0000 L CNN
F 2 "~" H 10388 5900 30  0000 C CNN
F 3 "~" H 10350 6050 60  0000 C CNN
	1    10350 6050
	0    -1   -1   0   
$EndComp
$Comp
L +5V #PWR020
U 1 1 5215B531
P 10600 5250
F 0 "#PWR020" H 10600 5340 20  0001 C CNN
F 1 "+5V" H 10600 5340 30  0000 C CNN
F 2 "" H 10600 5250 60  0000 C CNN
F 3 "" H 10600 5250 60  0000 C CNN
	1    10600 5250
	1    0    0    -1  
$EndComp
Text Label 10000 5850 2    60   ~ 0
FOOT_IN
Text Label 1050 2950 0    60   ~ 0
FOOT_IN
$Comp
L MCP4921-E/P U3
U 1 1 5215C032
P 6700 5700
F 0 "U3" H 6700 6050 40  0000 L BNN
F 1 "MCP4921-E/P" H 6700 6000 40  0000 L BNN
F 2 "DIP8" H 6700 5700 30  0000 C CIN
F 3 "" H 6700 5700 60  0000 C CNN
	1    6700 5700
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR021
U 1 1 5215C03F
P 6600 5250
F 0 "#PWR021" H 6600 5340 20  0001 C CNN
F 1 "+5V" H 6600 5340 30  0000 C CNN
F 2 "" H 6600 5250 60  0000 C CNN
F 3 "" H 6600 5250 60  0000 C CNN
	1    6600 5250
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR022
U 1 1 5215C045
P 6600 6150
F 0 "#PWR022" H 6600 6150 30  0001 C CNN
F 1 "GND" H 6600 6080 30  0001 C CNN
F 2 "" H 6600 6150 60  0000 C CNN
F 3 "" H 6600 6150 60  0000 C CNN
	1    6600 6150
	1    0    0    -1  
$EndComp
Wire Wire Line
	7550 1200 7800 1200
Wire Wire Line
	7550 1200 7550 950 
Wire Wire Line
	7800 1900 7550 1900
Wire Wire Line
	7550 1900 7550 2150
Wire Wire Line
	7950 950  8050 950 
Wire Wire Line
	8000 650  8000 1000
Connection ~ 8000 950 
Wire Wire Line
	8000 2100 8000 2150
Wire Wire Line
	7950 2150 8050 2150
Connection ~ 8000 2150
Wire Wire Line
	8000 1400 8000 1700
Wire Wire Line
	7750 1650 10150 1650
Wire Wire Line
	8900 1400 8900 1700
Connection ~ 8000 1650
Connection ~ 8900 1650
Wire Wire Line
	8450 1250 8450 1850
Connection ~ 8450 1650
Wire Wire Line
	8850 2150 8900 2150
Wire Wire Line
	8900 2100 8900 2200
Connection ~ 8900 2150
Wire Wire Line
	8900 900  8900 1000
Wire Wire Line
	8850 950  8900 950 
Connection ~ 8900 950 
Wire Wire Line
	9050 1650 9050 1700
Wire Wire Line
	7800 1200 7800 1900
Wire Wire Line
	7750 1450 7800 1450
Connection ~ 7800 1450
Wire Wire Line
	10100 950  10150 950 
Wire Wire Line
	10150 900  10150 1000
Wire Wire Line
	10150 1650 10150 1400
Connection ~ 9050 1650
Wire Wire Line
	9700 1250 9700 1650
Connection ~ 9700 1650
Wire Wire Line
	8000 650  9250 650 
Wire Wire Line
	9250 650  9250 950 
Wire Wire Line
	9250 950  9300 950 
Connection ~ 10150 950 
Wire Wire Line
	1050 950  1750 950 
Wire Wire Line
	1050 4250 1250 4250
Wire Wire Line
	1250 4250 1250 4200
Wire Wire Line
	1050 850  1450 850 
Wire Wire Line
	2000 1050 2800 1050
Wire Wire Line
	2000 1050 2000 1000
Wire Wire Line
	1450 1100 4500 1100
Wire Wire Line
	2300 1100 2300 1000
Wire Wire Line
	1050 1650 2100 1650
Wire Wire Line
	2100 1650 2100 1000
Wire Wire Line
	1050 1750 2200 1750
Wire Wire Line
	2200 1750 2200 1000
Wire Wire Line
	2650 1100 2650 1150
Connection ~ 2300 1100
Wire Wire Line
	1050 5250 1350 5250
Wire Wire Line
	2500 2650 2500 1000
Wire Wire Line
	2650 1050 2650 850 
Wire Wire Line
	1450 850  1450 1100
Wire Wire Line
	1200 850  1200 650 
Connection ~ 1200 850 
Wire Wire Line
	1850 650  1850 1150
Wire Wire Line
	1850 650  1800 650 
Wire Wire Line
	1850 1150 1050 1150
Wire Wire Line
	1050 1550 3000 1550
Wire Wire Line
	3000 1550 3000 1050
Wire Wire Line
	1050 1250 3100 1250
Wire Wire Line
	3100 1250 3100 1050
Wire Wire Line
	1050 1350 3600 1350
Wire Wire Line
	3600 1350 3600 1050
Wire Wire Line
	1050 1450 3400 1450
Wire Wire Line
	3400 1450 3400 1050
Connection ~ 2650 1100
Wire Wire Line
	3200 1100 3200 1050
Wire Wire Line
	3300 1050 3300 1200
Wire Wire Line
	3300 1200 2800 1200
Wire Wire Line
	2800 1200 2800 1050
Connection ~ 2650 1050
Wire Wire Line
	1750 950  1750 850 
Wire Wire Line
	4500 1100 4500 1050
Wire Wire Line
	4400 1050 4400 1100
Wire Wire Line
	4300 1050 4300 1200
Wire Wire Line
	4650 850  4650 1200
Wire Wire Line
	1050 2050 2650 2050
Wire Wire Line
	4100 1650 4100 1050
Wire Wire Line
	1050 2150 2750 2150
Wire Wire Line
	4200 1750 4200 1050
Wire Wire Line
	3650 3300 3650 3400
Wire Wire Line
	2950 3150 2900 3150
Wire Wire Line
	2900 3150 2900 3350
Wire Wire Line
	4400 3350 2900 3350
Connection ~ 3650 3350
Wire Wire Line
	3650 5050 3650 5150
Wire Wire Line
	4400 5100 2900 5100
Wire Wire Line
	2900 4400 2900 5100
Wire Wire Line
	2900 4900 2950 4900
Connection ~ 3650 5100
Wire Wire Line
	4100 1650 2650 1650
Wire Wire Line
	2750 1750 4200 1750
Wire Wire Line
	1050 2250 2850 2250
Wire Wire Line
	2850 2050 2950 2050
Wire Wire Line
	1050 2350 2950 2350
Wire Wire Line
	1050 2450 2850 2450
Wire Wire Line
	2850 2650 2950 2650
Wire Wire Line
	1050 2550 2700 2550
Wire Wire Line
	2850 2450 2850 2650
Wire Wire Line
	2850 2250 2850 2050
Wire Wire Line
	2700 3800 2950 3800
Wire Wire Line
	1050 2750 2600 2750
Wire Wire Line
	2600 4100 2950 4100
Wire Wire Line
	2900 4400 2950 4400
Connection ~ 2900 4900
Wire Wire Line
	4400 4400 4400 5100
Wire Wire Line
	4400 4700 4350 4700
Wire Wire Line
	4400 1950 4400 3350
Wire Wire Line
	4400 2950 4350 2950
Wire Wire Line
	3650 3650 3650 3750
Wire Wire Line
	3650 1900 3650 2000
Wire Wire Line
	4500 5000 4350 5000
Wire Wire Line
	4500 3050 4500 5000
Wire Wire Line
	4500 3050 4350 3050
Wire Wire Line
	4350 3150 4500 3150
Connection ~ 4500 3150
Wire Wire Line
	4350 3250 4500 3250
Connection ~ 4500 3250
Wire Wire Line
	4350 4900 4500 4900
Connection ~ 4500 4900
Wire Wire Line
	4350 4800 4500 4800
Connection ~ 4500 4800
Wire Wire Line
	1050 2850 2800 2850
Wire Wire Line
	2800 3500 4500 3500
Connection ~ 4500 3500
Wire Wire Line
	2600 2750 2600 4100
Wire Wire Line
	2700 2550 2700 3800
Wire Wire Line
	2800 2850 2800 3500
Wire Wire Line
	5200 2250 4950 2250
Wire Wire Line
	4950 2250 4950 2050
Wire Wire Line
	4950 2050 4350 2050
Wire Wire Line
	4350 2150 4850 2150
Wire Wire Line
	4850 2150 4850 2350
Wire Wire Line
	4850 2350 5200 2350
Wire Wire Line
	4350 2350 4750 2350
Wire Wire Line
	4750 2350 4750 2450
Wire Wire Line
	4750 2450 5200 2450
Wire Wire Line
	4350 2450 4650 2450
Wire Wire Line
	4650 2450 4650 2550
Wire Wire Line
	4650 2550 5200 2550
Wire Wire Line
	4350 2650 5200 2650
Wire Wire Line
	4350 2750 6050 2750
Wire Wire Line
	4350 3800 4650 3800
Wire Wire Line
	4650 3800 4650 2850
Wire Wire Line
	4650 2850 6150 2850
Wire Wire Line
	4350 3900 4750 3900
Wire Wire Line
	4750 3900 4750 2950
Wire Wire Line
	4750 2950 6250 2950
Wire Wire Line
	4350 4100 4850 4100
Wire Wire Line
	4850 4100 4850 3050
Wire Wire Line
	4850 3050 6350 3050
Wire Wire Line
	4350 4200 4950 4200
Wire Wire Line
	4950 4200 4950 3150
Wire Wire Line
	4950 3150 6450 3150
Connection ~ 4400 2950
Wire Wire Line
	5200 2150 5050 2150
Wire Wire Line
	5050 2150 5050 1750
Wire Wire Line
	2500 2650 1350 2650
Wire Wire Line
	1350 2650 1350 5250
Wire Wire Line
	4400 4500 4350 4500
Connection ~ 4400 4700
Wire Wire Line
	4400 4400 4350 4400
Connection ~ 4400 4500
Wire Wire Line
	2950 7250 2950 7150
Wire Wire Line
	2950 7150 6750 7150
Wire Wire Line
	2850 7250 2850 7100
Wire Wire Line
	2750 7250 2750 7050
Wire Wire Line
	5950 7250 5950 7150
Connection ~ 5950 7150
Wire Wire Line
	5850 7100 5850 7250
Wire Wire Line
	5750 7050 5750 7250
Wire Wire Line
	4450 7250 4450 7150
Connection ~ 4450 7150
Wire Wire Line
	4250 7050 4250 7250
Wire Wire Line
	4350 7100 4350 7250
Wire Wire Line
	1950 7250 1950 6950
Wire Wire Line
	6450 6300 6450 7250
Wire Wire Line
	6350 6900 6350 7250
Wire Wire Line
	1850 6150 1850 7250
Wire Wire Line
	1750 6200 1750 7250
Wire Wire Line
	6250 6850 6250 7250
Wire Wire Line
	6150 6800 6150 7250
Wire Wire Line
	1650 6250 1650 7250
Wire Wire Line
	4850 6900 4850 7250
Wire Wire Line
	4750 6850 4750 7250
Wire Wire Line
	4650 6800 4650 7250
Wire Wire Line
	3350 7250 3350 6900
Wire Wire Line
	3250 7250 3250 6850
Wire Wire Line
	5350 6700 5350 7250
Wire Wire Line
	2350 6700 2350 7250
Wire Wire Line
	2250 7250 2250 6650
Wire Wire Line
	5250 6650 5250 7250
Wire Wire Line
	2150 7250 2150 6600
Wire Wire Line
	5150 6600 5150 7250
Wire Wire Line
	3850 6700 3850 7250
Wire Wire Line
	3750 6650 3750 7250
Wire Wire Line
	3650 6600 3650 7250
Wire Wire Line
	1050 7450 1550 7450
Wire Wire Line
	1550 7450 1550 6700
Wire Wire Line
	1050 7250 1500 7250
Wire Wire Line
	1050 7150 1450 7150
Wire Wire Line
	1050 7050 1400 7050
Wire Wire Line
	6550 6500 6550 7250
Wire Wire Line
	9950 6450 10000 6450
Wire Wire Line
	10000 6450 10000 6400
Wire Wire Line
	10000 6400 10150 6400
Wire Wire Line
	10100 6450 10100 6400
Connection ~ 10100 6400
Wire Wire Line
	9950 6850 10000 6850
Wire Wire Line
	10000 6850 10000 6900
Wire Wire Line
	10000 6900 10650 6900
Wire Wire Line
	10100 6900 10100 6850
Wire Wire Line
	8950 6850 9050 6850
Wire Wire Line
	8450 6550 9050 6550
Wire Wire Line
	8350 6450 8350 6650
Wire Wire Line
	8950 6450 9050 6450
Wire Wire Line
	9000 6400 9000 6450
Wire Wire Line
	8350 6650 9050 6650
Wire Wire Line
	9000 6850 9000 6900
Wire Wire Line
	6650 6900 6650 7250
Wire Wire Line
	8350 6450 8450 6450
Connection ~ 9000 6450
Connection ~ 9000 6850
Wire Wire Line
	8450 6850 8450 6550
Wire Wire Line
	1500 7250 1500 6650
Wire Wire Line
	1450 7150 1450 6600
Wire Wire Line
	1400 7050 1400 6500
Wire Wire Line
	1600 7100 6950 7100
Connection ~ 4350 7100
Connection ~ 5850 7100
Connection ~ 6850 7100
Wire Wire Line
	2750 7050 6650 7050
Connection ~ 4250 7050
Connection ~ 5750 7050
Connection ~ 6650 7050
Wire Wire Line
	1950 6950 6450 6950
Wire Wire Line
	3450 7250 3450 6950
Connection ~ 3450 6950
Wire Wire Line
	4950 7250 4950 6950
Connection ~ 4950 6950
Wire Wire Line
	1850 6900 6350 6900
Connection ~ 3350 6900
Connection ~ 4850 6900
Wire Wire Line
	1750 6850 6250 6850
Connection ~ 3250 6850
Connection ~ 4750 6850
Wire Wire Line
	1650 6800 6150 6800
Wire Wire Line
	3150 6800 3150 7250
Connection ~ 4650 6800
Connection ~ 3150 6800
Wire Wire Line
	1550 6700 5850 6700
Connection ~ 2350 6700
Connection ~ 3850 6700
Wire Wire Line
	1500 6650 5950 6650
Connection ~ 2250 6650
Connection ~ 3750 6650
Wire Wire Line
	1450 6600 6050 6600
Connection ~ 2150 6600
Connection ~ 3650 6600
Wire Wire Line
	1400 6500 6550 6500
Wire Wire Line
	1050 6550 1350 6550
Wire Wire Line
	1350 6550 1350 6450
Wire Wire Line
	1350 6450 2050 6450
Wire Wire Line
	2050 6450 2050 7250
Wire Wire Line
	1050 6650 1300 6650
Wire Wire Line
	1300 6650 1300 6400
Wire Wire Line
	1300 6400 3550 6400
Wire Wire Line
	3550 6400 3550 7250
Wire Wire Line
	1050 6750 1250 6750
Wire Wire Line
	1250 6750 1250 6350
Wire Wire Line
	5050 6350 5050 7250
Wire Wire Line
	1250 6350 5050 6350
Wire Wire Line
	1050 5650 2550 5650
Wire Wire Line
	2550 5650 2550 7250
Wire Wire Line
	1050 5750 2450 5750
Wire Wire Line
	2450 5750 2450 7250
Wire Wire Line
	1050 6450 1200 6450
Wire Wire Line
	1200 6450 1200 6250
Wire Wire Line
	1200 6250 1650 6250
Connection ~ 1650 6800
Wire Wire Line
	1050 6350 1150 6350
Wire Wire Line
	1150 6350 1150 6200
Wire Wire Line
	1150 6200 1750 6200
Connection ~ 1750 6850
Wire Wire Line
	1050 6250 1100 6250
Wire Wire Line
	1100 6250 1100 6150
Wire Wire Line
	1100 6150 1850 6150
Connection ~ 1850 6900
Wire Wire Line
	1050 5850 4050 5850
Wire Wire Line
	4050 5850 4050 7250
Wire Wire Line
	1050 5950 3950 5950
Wire Wire Line
	3950 5950 3950 7250
Wire Wire Line
	1050 6050 5550 6050
Wire Wire Line
	5550 6050 5550 7250
Wire Wire Line
	1050 3250 1550 3250
Wire Wire Line
	1550 5550 5450 5550
Wire Wire Line
	5450 5550 5450 7250
Wire Wire Line
	10600 6100 10600 6050
Wire Wire Line
	10550 6050 10650 6050
Wire Wire Line
	10550 5850 10650 5850
Wire Wire Line
	10600 5850 10600 5800
Connection ~ 10600 5850
Connection ~ 10600 6050
Wire Wire Line
	10600 5300 10600 5250
Wire Wire Line
	10150 6050 10000 6050
Wire Wire Line
	10000 6050 10000 5850
Wire Wire Line
	10000 5850 10050 5850
Connection ~ 10100 6900
Wire Wire Line
	10650 6700 10650 6700
Wire Wire Line
	10650 6700 10650 6400
Wire Wire Line
	6050 6600 6050 5750
Wire Wire Line
	6050 5750 6150 5750
Connection ~ 5150 6600
Wire Wire Line
	5950 6650 5950 5650
Wire Wire Line
	5950 5650 6150 5650
Connection ~ 5250 6650
Wire Wire Line
	5850 6700 5850 5550
Wire Wire Line
	5850 5550 6150 5550
Connection ~ 5350 6700
Wire Wire Line
	1550 3250 1550 5550
Wire Wire Line
	1450 5450 5750 5450
Wire Wire Line
	5750 5450 5750 5850
Wire Wire Line
	5750 5850 6150 5850
Wire Wire Line
	1050 3350 1450 3350
Wire Wire Line
	1450 3350 1450 5450
Wire Wire Line
	6600 5300 6600 5250
Wire Wire Line
	6600 6100 6600 6150
Wire Wire Line
	7350 6300 7350 5700
Wire Wire Line
	7350 5700 7250 5700
Wire Wire Line
	6450 6300 7350 6300
Connection ~ 6450 6950
$Comp
L POT RV1
U 1 1 5215D7FC
P 7600 6150
F 0 "RV1" H 7600 6050 50  0000 C CNN
F 1 "10k" H 7600 6150 50  0000 C CNN
F 2 "~" H 7600 6150 60  0000 C CNN
F 3 "~" H 7600 6150 60  0000 C CNN
	1    7600 6150
	0    -1   -1   0   
$EndComp
$Comp
L R R1
U 1 1 5215D818
P 7600 6700
F 0 "R1" V 7680 6700 40  0000 C CNN
F 1 "33k" V 7607 6701 40  0000 C CNN
F 2 "~" V 7530 6700 30  0000 C CNN
F 3 "~" H 7600 6700 30  0000 C CNN
	1    7600 6700
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR023
U 1 1 5215D83C
P 7600 7000
F 0 "#PWR023" H 7600 7000 30  0001 C CNN
F 1 "GND" H 7600 6930 30  0001 C CNN
F 2 "" H 7600 7000 60  0000 C CNN
F 3 "" H 7600 7000 60  0000 C CNN
	1    7600 7000
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR024
U 1 1 5215D851
P 7600 5850
F 0 "#PWR024" H 7600 5940 20  0001 C CNN
F 1 "+5V" H 7600 5940 30  0000 C CNN
F 2 "" H 7600 5850 60  0000 C CNN
F 3 "" H 7600 5850 60  0000 C CNN
	1    7600 5850
	1    0    0    -1  
$EndComp
Wire Wire Line
	6800 6100 6800 6150
Wire Wire Line
	6800 6150 7450 6150
Wire Wire Line
	7600 7000 7600 6950
Wire Wire Line
	7600 6450 7600 6400
Wire Wire Line
	7600 5900 7600 5850
$Comp
L +3.3V #PWR025
U 1 1 5215E5B4
P 9000 6400
F 0 "#PWR025" H 9000 6360 30  0001 C CNN
F 1 "+3.3V" H 9000 6510 30  0000 C CNN
F 2 "" H 9000 6400 60  0000 C CNN
F 3 "" H 9000 6400 60  0000 C CNN
	1    9000 6400
	1    0    0    -1  
$EndComp
$Comp
L C C1
U 1 1 5215EBBD
P 7250 2900
F 0 "C1" H 7250 3000 40  0000 L CNN
F 1 "100n" H 7256 2815 40  0000 L CNN
F 2 "~" H 7288 2750 30  0000 C CNN
F 3 "~" H 7250 2900 60  0000 C CNN
	1    7250 2900
	1    0    0    -1  
$EndComp
$Comp
L C C2
U 1 1 5215EBCA
P 7450 2900
F 0 "C2" H 7450 3000 40  0000 L CNN
F 1 "100n" H 7456 2815 40  0000 L CNN
F 2 "~" H 7488 2750 30  0000 C CNN
F 3 "~" H 7450 2900 60  0000 C CNN
	1    7450 2900
	1    0    0    -1  
$EndComp
$Comp
L C C3
U 1 1 5215EBD0
P 7650 2900
F 0 "C3" H 7650 3000 40  0000 L CNN
F 1 "100n" H 7656 2815 40  0000 L CNN
F 2 "~" H 7688 2750 30  0000 C CNN
F 3 "~" H 7650 2900 60  0000 C CNN
	1    7650 2900
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR026
U 1 1 5215EBD8
P 7250 3200
F 0 "#PWR026" H 7250 3200 30  0001 C CNN
F 1 "GND" H 7250 3130 30  0001 C CNN
F 2 "" H 7250 3200 60  0000 C CNN
F 3 "" H 7250 3200 60  0000 C CNN
	1    7250 3200
	1    0    0    -1  
$EndComp
Wire Wire Line
	7250 3100 7250 3200
Wire Wire Line
	7250 3150 7650 3150
Wire Wire Line
	7450 3150 7450 3100
Connection ~ 7250 3150
Wire Wire Line
	7650 3150 7650 3100
Connection ~ 7450 3150
Wire Wire Line
	7250 2600 7250 2700
Wire Wire Line
	7250 2650 7650 2650
Wire Wire Line
	7450 2650 7450 2700
Connection ~ 7250 2650
Wire Wire Line
	7650 2650 7650 2700
Connection ~ 7450 2650
Wire Wire Line
	4650 1200 4300 1200
Wire Wire Line
	2750 2150 2750 1750
Wire Wire Line
	2650 1650 2650 2050
$Comp
L +5V #PWR027
U 1 1 521605F5
P 8900 900
F 0 "#PWR027" H 8900 990 20  0001 C CNN
F 1 "+5V" H 8900 990 30  0000 C CNN
F 2 "" H 8900 900 60  0000 C CNN
F 3 "" H 8900 900 60  0000 C CNN
	1    8900 900 
	1    0    0    -1  
$EndComp
Text Label 7550 1900 2    60   ~ 0
AC_IN
Text Label 8000 650  0    60   ~ 0
UNR+
Text Label 8000 2150 3    60   ~ 0
UNR-
$Comp
L CONN_6X2 P9
U 1 1 52164F61
P 5600 2400
F 0 "P9" H 5600 2750 60  0000 C CNN
F 1 "TEN_POTS" V 5600 2400 60  0000 C CNN
F 2 "" H 5600 2400 60  0000 C CNN
F 3 "" H 5600 2400 60  0000 C CNN
	1    5600 2400
	1    0    0    -1  
$EndComp
Wire Wire Line
	4400 1950 6050 1950
Wire Wire Line
	6050 1950 6050 2150
Wire Wire Line
	6050 2150 6000 2150
Wire Wire Line
	6050 2750 6050 2650
Wire Wire Line
	6050 2650 6000 2650
Wire Wire Line
	6150 2850 6150 2550
Wire Wire Line
	6150 2550 6000 2550
Wire Wire Line
	6250 2950 6250 2450
Wire Wire Line
	6250 2450 6000 2450
Wire Wire Line
	6350 3050 6350 2350
Wire Wire Line
	6350 2350 6000 2350
Wire Wire Line
	6450 3150 6450 2250
Wire Wire Line
	6450 2250 6000 2250
Wire Wire Line
	6750 6900 6750 7250
Connection ~ 6750 7150
Wire Wire Line
	6850 7100 6850 7250
Wire Wire Line
	6950 6900 6950 7250
Wire Wire Line
	6950 6900 7100 6900
Wire Wire Line
	7100 6900 7100 7000
Connection ~ 6950 7100
Wire Wire Line
	1050 6850 1600 6850
Wire Wire Line
	1600 6850 1600 7100
Connection ~ 2850 7100
Connection ~ 4400 1100
Connection ~ 3200 1100
$EndSCHEMATC
