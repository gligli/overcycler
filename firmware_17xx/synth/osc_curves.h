#ifndef OSC_CURVES_H
#define	OSC_CURVES_H

// add 32768 to the value, and you get the frequency of the 12th octave in 1/20th of semitones for A = 440Hz
const uint16_t oscOctaveCurve[256]=
{
	720,817,914,1011,1109,1207,1305,1404,1503,1602,
	1701,1801,1901,2001,2102,2203,2304,2405,2507,2609,
	2711,2814,2917,3020,3124,3227,3332,3436,3541,3646,
	3751,3857,3963,4069,4175,4282,4389,4497,4605,4713,
	4821,4930,5039,5148,5258,5368,5478,5589,5700,5811,
	5923,6034,6147,6259,6372,6485,6599,6713,6827,6941,
	7056,7171,7287,7403,7519,7636,7752,7870,7987,8105,
	8223,8342,8461,8580,8699,8819,8940,9060,9181,9303,
	9424,9546,9669,9791,9915,10038,10162,10286,10411,10535,
	10661,10786,10912,11039,11165,11292,11420,11548,11676,11804,
	11933,12063,12192,12322,12453,12583,12715,12846,12978,13110,
	13243,13376,13510,13643,13778,13912,14047,14183,14319,14455,
	14591,14728,14866,15003,15142,15280,15419,15558,15698,15838,
	15979,16120,16261,16403,16545,16688,16831,16975,17118,17263,
	17407,17553,17698,17844,17990,18137,18284,18432,18580,18729,
	18878,19027,19177,19327,19478,19629,19780,19932,20085,20238,
	20391,20545,20699,20854,21009,21164,21320,21477,21634,21791,
	21949,22107,22266,22425,22584,22745,22905,23066,23228,23390,
	23552,23715,23878,24042,24206,24371,24536,24702,24868,25035,
	25202,25370,25538,25707,25876,26045,26216,26386,26557,26729,
	26901,27074,27247,27420,27594,27769,27944,28120,28296,28472,
	28649,28827,29005,29184,29363,29543,29723,29904,30085,30267,
	30449,30632,30815,30999,31184,31369,31554,31740,31927,32114,
	32301,32490,32678,32868,33058,33248,33439,33630,33822,34015,
	34208,34402,34596,34791,34986,35182,35379,35576,35774,35972,
	36171,36370,36570,36771,36972,37173,
};

#endif	/* OSC_CURVES_H */
