#error Documentation only header. Do not include.

/** \mainpage The front-end optimizer for the Bobox framework.
 *
 *  This standalone tool implemented on top of the Clang tooling interface
 *  analyses code for the Bobox framework and tries to reduce a number of 
 *  short-running and long-running tasks.
 * 
 *  The tool implements the prefetch optimization method
 *  \link bobopt::methods::prefetch \endlink that removes short-running tasks
 *  which are caused by scheduling task even though it does not have all input
 *  data. See bench_prefetch and bench_prefetch_attack benchmarks for an
 *  example of such code.
 * 
 *  The tool implements the yield complex optimization method
 *  \link bobopt::methods::yield_complex \endlink that removes long-running
 *  paths in general. Analysis looks for too complex paths in CFG based on
 *  some predefined values and tries to place yield() into code so it does
 *  make new CFG better than the previous CFG. See thesis text for CFG quuality
 *  evaluation. See bench_yield_complex benchmark for an example of code that
 *  is greatly optimized by this optimization method.
 * 
 *  Compilation tested with the Microsoft Visual C++ Compiler Nov 2012 CTP. GCC
 *  and Clang have to support the STL <regex> library. However, compilation
 *  works even on Clang and GCC.
 * 
 *  Should be compiled and linked against: <BR>
 *  LLVM: revision 217320 (22:44:41, Saturday, September 06, 2014) <BR>
 *  Clang: revision 217314 (20:16:37, Saturday, September 06, 2014) <BR>
 */