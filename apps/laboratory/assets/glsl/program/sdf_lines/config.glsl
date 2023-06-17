#define LOCAL_SIZE 8

const int line_size = 32;
#ifdef ONLY_COMP
const uint line_base = gl_LocalInvocationIndex * line_size*3;
#endif

const int ring_res = 6;
const int ring_count = line_size - 2;
const int tube_size = 2 * 1/*head & tail*/ + ring_count * ring_res/*body*/;
#ifdef ONLY_COMP
const uint tube_base = gl_LocalInvocationIndex * tube_size*3;
#endif
