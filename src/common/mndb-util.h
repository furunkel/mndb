#pragma once

#define MAX(a,b) ((a > b) ?  (a) : (b))
#define MIN(a,b) ((a < b) ?  (a) : (b))

#define unlikely(e) (__builtin_expect(e, 0))
#define likely(e) (__builtin_expect(e, 0))
