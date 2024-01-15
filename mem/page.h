#include "file.h"

namespace mem {
class Page {};

struct FreeListNode {
    Page page;
    Offset previous;
    Offset next;
};
}  // namespace mem