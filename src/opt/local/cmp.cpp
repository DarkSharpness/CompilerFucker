#include "peephole.h"


namespace dark::OPT {

/**
 * Strength reduction:
 * X != 1   --> (X == 0)        // iff boolean
 * X == 1   --> (X != 0) = X    // iff boolean
 * X <= Y   --> (Y >= X)
 * X >  Y   --> (Y <  X)
 * 
 * General case:
 * (X != Y) == 0    --> (X == Y)
 * (X == Y) == 0    --> (X != Y)
 * (X <  Y) == 0    --> (X >= Y)
 * (X >= Y) == 0    --> (X <  Y)
 * 
 * Booleans only:
 * (X != Y) != Y    --> (X != 0) = X
 * (X == Y) == Y    --> (X != 0) = X
 * (X == Y) != Y    --> (X == 0)
 * (X != Y) == Y    --> (X == 0)
 * 
 * 
 * Branch opt: (asm)
 * %tmp = icmp ...
 * br %tmp ..
 * --> b... (asm)
 * 
 * 
*/


}