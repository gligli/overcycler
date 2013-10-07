/*------------------------------------------------------------------------------

   HXA7241 General library.
   Harrison Ainsworth / HXA7241 : 2004-2011

   http://www.hxa.name/

------------------------------------------------------------------------------*/


#ifndef PowFast_h
#define PowFast_h




/**
 * Fast approximation to pow, with adjustable precision.<br/><br/>
 *
 * Precision can be 0 to 18.<br/>
 * Storage is (2 ^ precision) * 4 bytes -- 4B to 1MB<br/>
 * For precision 11: mean error < 0.01%, max error < 0.02%, storage 8KB.
 */


/**
 * Initialize powFast lookup table.
 *
 * @pTable     length must be 2 ^ precision
 * @precision  number of mantissa bits used, >= 0 and <= 18
 */
void powFastSetTable
(
   unsigned int* pTable,
   const unsigned int  precision
);

float powFast2_simple
(
   unsigned int*	   pTable,
   const unsigned int  precision,
   float       f
);

#if 0

/*- queries ------------------------------------------------------------------*/

/**
 * Get 2 ^ number.<br/><br/>
 *
 * @powFast  object from the powFastCreate function
 * @f        power to apply, must be > -125 and < +128.
 *
 * @return  2 ^ f
 */
float powFast2
(
   const void* powFast,
   float       f
);


/**
 * Get e ^ number.<br/><br/>
 *
 * @powFast  object from the powFastCreate function
 * @f        power to apply, must be > -87.3ish and < +88.7ish
 *
 * @return  e ^ f
 */
float powFastE
(
   const void* powFast,
   float       f
);


/**
 * Get 10 ^ number.<br/><br/>
 *
 * @powFast  object from the powFastCreate function
 * @f        power to apply, must be > -37.9ish and < +38.5ish
 *
 * @return  10 ^ f
 */
float powFast10
(
   const void* powFast,
   float       f
);


/**
 * Get r ^ number.<br/><br/>
 *
 * @powFast  object from the powFastCreate function
 * @logr     logE of radix for power
 * @f        power to apply (beware under/over-flow)
 *
 * @return  r ^ f
 */
float powFast
(
   const void* powFast,
   float       logr,
   float       f
);


/**
 * Get precision.<br/><br/>
 *
 * @powFast  object from the powFastCreate function
 *
 * @return  precision
 */
unsigned int powFastPrecision
(
   const void* powFast
);

#endif


#endif/*PowFast_h*/
