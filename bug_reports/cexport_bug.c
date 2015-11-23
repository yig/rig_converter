// brew install assimp
// cc cexport_bug.c -o cexport_bug -I/usr/local/include -L/usr/local/lib -lassimp

#include <stdio.h>
#include <assimp/cexport.h>

int main( int argc, char* argv[] ) {
    // Get the count
    const size_t count = aiGetExportFormatCount();
    for( size_t i = 0; i < count; ++i )
    {
        // Get the description.
        const struct aiExportFormatDesc* desc = aiGetExportFormatDescription( i );
        
        // desc points to free'd memory. The following crashes or prints garbage.
        printf( "%s: %s (id: %s)\n", desc->fileExtension, desc->description, desc->id );
    }
    
    return 0;
}
