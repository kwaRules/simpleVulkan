#pragma once

#include <fstream>
#include "XSArray.h"

class XSShader 
{
    public:
    XSArray<char> code;
    XSShader( const char *path );
};