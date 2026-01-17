#ifndef __PHY_LINE_RB__
#define __PHY_LINE_RB__

#include "vec2.h"

namespace phy {

    struct LineRb {
        vec2 startPos, endPos;

        vec2 getDir() {
            return endPos - startPos;
        }
    };

}


#endif 