// c++ converter.cpp -o converter -I/usr/local/include -L/usr/local/lib -lassimp -g -Wall

#define SAVE_RIG 1

#include <iostream>
#include <fstream>
#include <algorithm> // pair
#include <cassert>

#include <assimp/Exporter.hpp>
#include <assimp/cimport.h>
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
    
    Assimp::Exporter exporter;
    const size_t count = exporter.GetExportFormatCount();
    
    for( size_t i = 0; i < count; ++i )
    {
        const aiExportFormatDesc* desc = exporter.GetExportFormatDescription( i );
        assert( desc );
        
        out << desc->fileExtension << ": " << desc->description << " (id: " << desc->id << ")" << std::endl;
    }
}

const char* IdFromExtension( const std::string& extension )
{
    Assimp::Exporter exporter;
    const size_t count = exporter.GetExportFormatCount();
    
    for( size_t i = 0; i < count; ++i )
    {
        const aiExportFormatDesc* desc = exporter.GetExportFormatDescription( i );
        assert( desc );
        
        if( extension == desc->fileExtension )
        {
            std::cout << "Found a match for extension: " << extension << std::endl;
            std::cout << desc->fileExtension << ": " << desc->description << " (id: " << desc->id << ")" << std::endl;
            
            return desc->id;
        }
    }
    
    return nullptr;
}

#if SAVE_RIG
void save_rig( const aiMesh* mesh )
{
    // Iterate over the bones of the mesh.
    for( int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index ) {
        const aiBone* bone = mesh->mBones[ bone_index ];
        
        // Save the vertex weights for the bone.
        // Lookup the index for the bone by its name.
    }
}

void save_rig( const aiScene* scene )
{
    printf( "# Saving the rig.\n" );
    
    // TODO: Save the bones. There should be some kind of nodes with their
    //       names and positions. (We might also need the offsetmatrix from the bone itself).
    //       Store a map of name to index.
    //       See (maybe): http://ogldev.atspace.co.uk/www/tutorial38/tutorial38.html
    /*
import pyassimp
filename = '/Users/yotam/Work/ext/three.js/examples/models/collada/monster/monster.dae'
scene = pyassimp.load( filename )
spaces = 0
def recurse( node ):
    global spaces
    print (' '*spaces), node.name #, node.transformation
    spaces += 1
    for child in node.children: recurse( child )
    spaces -= 1

recurse( scene.rootnode )
    */
    
    // Meshes have bones. Iterate over meshes.
    for( int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index ) {
        printf( "Mesh %d\n", mesh_index );
        save_rig( scene->mMeshes[ mesh_index ] )
    }
}
#endif

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
    /// We need three arguments: the program, the input path, and the output path.
    if( 3 != argc ) {
        usage( argv[0], std::cerr );
        return -1;
    }
    
    /// Store the input and output paths.
    const char* inpath = argv[1];
    const char* outpath = argv[2];
    
    // Exit if the output path already exists.
    if( os_path_exists( outpath ) ) {
        std::cerr << "ERROR: Output path exists. Not clobbering: " << outpath << std::endl;
        usage( argv[0], std::cerr );
        return -1;
    }
    
    /// Get the extension of the output path and its corresponding ASSIMP id.
    std::string extension = os_path_splitext( outpath ).second;
    // os_path_splitext.second returns an extension of the form ".obj".
    // We want the substring from position 1 to the end.
    if( extension.size() <= 1 ) {
        std::cerr << "ERROR: No extension detected on the output path: " << extension << std::endl;
        usage( argv[0], std::cerr );
        return -1;
    }
    extension = extension.substr(1);
    
    const char* exportId = IdFromExtension( extension );
    // Exit if we couldn't find a corresponding ASSIMP id.
    if( nullptr == exportId ) {
        std::cerr << "ERROR: Output extension unsupported: " << extension << std::endl;
        usage( argv[0], std::cerr );
        return -1;
    }
    
    /// Load the scene.
    const aiScene* scene = aiImportFile( inpath, 0 );
    if( nullptr == scene ) {
        std::cerr << "ERROR: " << aiGetErrorString() << std::endl;
    }
    std::cout << "Loaded: " << inpath << std::endl;
    
    /// Save the scene.
    const aiReturn result = aiExportScene( scene, exportId, outpath, 0 );
    if( aiReturn_SUCCESS != result ) {
        std::cerr << "ERROR: Could not save the scene: " << ( (aiReturn_OUTOFMEMORY == result) ? "Out of memory" : "Unknown reason" ) << std::endl;
    }
    std::cout << "Saved: " << outpath << std::endl;
    
#if SAVE_RIG
    /// Save the rig.
    save_rig( scene );
#endif
    
    // Cleanup.
    aiReleaseImport( scene );
    
    return 0;
}
