#include <iostream>
#include "Render/XSDisplay.h"

int
main() 
{
    XSDisplay DP_1;

    while( true ) 
    {
        DP_1.drawFrame();
    }

    return 0;
}