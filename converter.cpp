// c++ converter.cpp -o converter -I/usr/local/include -L/usr/local/lib -lassimp -g -Wall

#include <iostream>

// #include <assimp/cimport.h>
// #include <assimp/cexport.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void print_importers( std::ostream& out )
{
    out << "## Importers:\n";
    
    Assimp::Importer importer;
    
    aiString s;
    importer.GetExtensionList( s );
    
    out << s.C_Str() << std::endl;
}

void print_exporters( std::ostream& out )
{
    out << "## Exporters:\n";
    
    Assimp::Exporter exporter;
    
    const size_t count = exporter.GetExportFormatCount();
    for( size_t i = 0; i < count; ++i )
    {
        const aiExportFormatDesc* desc = exporter.GetExportFormatDescription( i );
        
        out << desc->fileExtension << ": " << desc->description << " (id: " << desc->id << ")" << std::endl;
    }
}

void usage( const char* argv0, std::ostream& out )
{
    out << "Usage: " << argv0 << " path/to/input path/to/output" << std::endl;
    
    out << std::endl;
    print_importers( out );
    
    out << std::endl;
    print_exporters( out );
}

int main( int argc, char* argv[] )
{
    if( 3 != argc )
    {
        usage( argv[0], std::cerr );
        
        return -1;
    }
    
    const char* inpath = argv[1];
    const char* outpath = argv[2];
    
    
    
    return 0;
}
