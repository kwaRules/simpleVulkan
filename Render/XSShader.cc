#include "XSShader.h"

XSShader::XSShader( const char *path ) 
{
    std::ifstream file( path, std::ios::ate | std::ios::binary );
    if( !file.is_open() ) 
    {
        printf( "Failed to read shader from path: %s\n", path );
        exit( 1 );
    }

    size_t fileSize = (size_t) file.tellg();
    code.resize( fileSize );
    file.seekg( 0 );
    file.read( code.data(), fileSize );
    file.close();
}