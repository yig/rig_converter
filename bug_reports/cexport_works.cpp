// brew install assimp
// c++ cexport_works.c -o cexport_works -I/usr/local/include -L/usr/local/lib -lassimp

#include <stdio.h>
#include <assimp/Exporter.hpp>

int main( int argc, char* argv[] ) {
    // Allocate an Exporter.
    Assimp::Exporter exporter;
    
    // Get the count
    const size_t count = exporter.GetExportFormatCount();
    for( size_t i = 0; i < count; ++i )
    {
        // Get the description.
        const aiExportFormatDesc* desc = exporter.GetExportFormatDescription( i );
        
        // desc does not point to free'd memory. The following works.
        printf( "%s: %s (id: %s)\n", desc->fileExtension, desc->description, desc->id );
    }
    
    return 0;
}
