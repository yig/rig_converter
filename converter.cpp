// c++ converter.cpp -o converter -I/usr/local/include -L/usr/local/lib -lassimp -g -Wall

#include <iostream>
#include <fstream>
#include <algorithm> // pair
#include <cassert>

#include <assimp/cimport.h>
#include <assimp/cexport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace
{
/// From my: stl.h

// Behaves like the python os.path.split() function.
inline std::pair< std::string, std::string > os_path_split( const std::string& path )
{
    const std::string::size_type split = path.find_last_of( "/" );
    if( split == std::string::npos )
        return std::make_pair( std::string(), path );
    else
    {
        std::string::size_type split_start = split;
        // Remove multiple trailing slashes.
        while( split_start > 0 && path[ split_start-1 ] == '/' ) split_start -= 1;
        // Don't remove the leading slash.
        if( split_start == 0 ) split_start = 1;
        return std::make_pair( path.substr( 0, split_start ), path.substr( split+1 ) );
    }
}

// Behaves like the python os.path.splitext() function.
inline std::pair< std::string, std::string > os_path_splitext( const std::string& path )
{
    const std::string::size_type split_dot = path.find_last_of( "." );
    const std::string::size_type split_slash = path.find_last_of( "/" );
    if( split_dot != std::string::npos && (split_slash == std::string::npos || split_slash < split_dot) )
        return std::make_pair( path.substr( 0, split_dot ), path.substr( split_dot ) );
    else
        return std::make_pair( path, std::string() );
}


/// From: http://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
bool os_path_exists( const std::string& name ) {
    std::ifstream f(name.c_str());
    if (f.good()) {
        f.close();
        return true;
    } else {
        f.close();
        return false;
    }   
}
}

void print_importers( std::ostream& out )
{
    out << "## Importers:\n";
    
    aiString s;
    aiGetExtensionList( &s );
    
    out << s.C_Str() << std::endl;
}

void print_exporters( std::ostream& out )
{
    out << "## Exporters:\n";
    
    const size_t count = aiGetExportFormatCount();
    for( size_t i = 0; i < count; ++i )
    {
        const aiExportFormatDesc* desc = aiGetExportFormatDescription( i );
        assert( desc );
        
        out << desc->fileExtension << ": " << desc->description << " (id: " << desc->id << ")" << std::endl;
    }
}

const char* IdFromExtension( const std::string& extension )
{
    const size_t count = aiGetExportFormatCount();
    for( size_t i = 0; i < count; ++i )
    {
        const aiExportFormatDesc* desc = aiGetExportFormatDescription( i );
        
        if( extension == desc->fileExtension )
        {
            std::cout << "Found a match for extension: " << extension << std::endl;
            std::cout << desc->fileExtension << ": " << desc->description << " (id: " << desc->id << ")" << std::endl;
            
            return desc->id;
        }
    }
    
    return nullptr;
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
    // We need three arguments: the program, the input path, and the output path.
    if( 3 != argc )
    {
        usage( argv[0], std::cerr );
        return -1;
    }
    
    // Store the input and output paths.
    const char* inpath = argv[1];
    const char* outpath = argv[2];
    
    // Exit if the output path already exists.
    if( os_path_exists( outpath ) )
    {
        std::cerr << "ERROR: Output path exists. Not clobbering: " << outpath << std::endl;
        usage( argv[0], std::cerr );
        return -1;
    }
    
    // Get the extension of the output path and its corresponding ASSIMP id.
    const std::string extension = os_path_splitext( outpath ).second;
    const char* exportId = IdFromExtension( extension );
    // Exit if we couldn't find a corresponding ASSIMP id.
    if( nullptr == exportId )
    {
        std::cerr << "ERROR: Output extension unsupported: " << extension << std::endl;
        usage( argv[0], std::cerr );
        return -1;
    }
    
    // Load the scene.
    const aiScene* scene = aiImportFile( inpath, 0 );
    if( nullptr == scene )
    {
        std::cerr << "ERROR: " << aiGetErrorString() << std::endl;
    }
    std::cout << "Loaded: " << inpath << std::endl;
    
    // Save the scene.
    const aiReturn result = aiExportScene( scene, exportId, outpath, 0 );
    if( aiReturn_SUCCESS != result )
    {
        std::cerr << "ERROR: Could not save the scene: " << ( (aiReturn_OUTOFMEMORY == result) ? "Out of memory" : "Unknown reason" ) << std::endl;
    }
    std::cout << "Saved: " << outpath << std::endl;
    
    // Cleanup.
    aiReleaseImport( scene );
    
    return 0;
}
