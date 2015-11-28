// c++ -std=c++11 converter.cpp -o converter -I/usr/local/include -L/usr/local/lib -lassimp -g -Wall

#define SAVE_RIG 1

#include <iostream>
#include <fstream>
#include <algorithm> // pair
#include <unordered_map>
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
    assert( mesh );
    
    // Iterate over the bones of the mesh.
    for( int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index ) {
        const aiBone* bone = mesh->mBones[ bone_index ];
        
        // Save the vertex weights for the bone.
        // Lookup the index for the bone by its name.
    }
}

aiMatrix4x4 FilterNodeTransformation( const aiMatrix4x4& transformation ) {
    return transformation;
    
    // Decompose the transformation matrix into translation, rotation, and scaling.
    aiVector3D scaling, translation_vector;
    aiQuaternion rotation;
    transformation.Decompose( scaling, rotation, translation_vector );
    rotation.Normalize();
    
    // Convert the translation into a matrix.
    aiMatrix4x4 translation_matrix;
    aiMatrix4x4::Translation( translation_vector, translation_matrix );
    
    // Keep the rotation times the translation.
    aiMatrix4x4 keep( aiMatrix4x4( rotation.GetMatrix() ) * translation_matrix );
    return keep;
}

typedef std::unordered_map< std::string, aiMatrix4x4 > StringToMatrix4x4;
// typedef std::unordered_map< std::string, std::string > StringToString;
void recurse( const aiNode* node, const aiMatrix4x4& parent_transformation, StringToMatrix4x4& name2transformation ) {
    assert( node );
    assert( name2transformation.find( node->mName.C_Str() ) == name2transformation.end() );
    
    const aiMatrix4x4 node_transformation = FilterNodeTransformation( node->mTransformation );
    const aiMatrix4x4 transformation_so_far = parent_transformation * node_transformation;
    
    name2transformation[ node->mName.C_Str() ] = transformation_so_far;
    
    for( int child_index = 0; child_index < node->mNumChildren; ++child_index ) {
        recurse( node->mChildren[ child_index ], transformation_so_far, name2transformation );
    }
}
void print( const StringToMatrix4x4& name2transformation ) {
    for( const auto& iter : name2transformation ) {
        std::cout << iter.first << ":\n";
        
        std::cout << ' ' << iter.second.a1 << ' ' << iter.second.a2 << ' ' << iter.second.a3 << ' ' << iter.second.a4 << '\n';
        std::cout << ' ' << iter.second.b1 << ' ' << iter.second.b2 << ' ' << iter.second.b3 << ' ' << iter.second.b4 << '\n';
        std::cout << ' ' << iter.second.c1 << ' ' << iter.second.c2 << ' ' << iter.second.c3 << ' ' << iter.second.c4 << '\n';
        std::cout << ' ' << iter.second.d1 << ' ' << iter.second.d2 << ' ' << iter.second.d3 << ' ' << iter.second.d4 << '\n';
    }
}

void save_rig( const aiScene* scene )
{
    printf( "# Saving the rig.\n" );
    
    assert( scene );
    assert( scene->mRootNode );
    
    StringToMatrix4x4 node2transformation;
    const aiMatrix4x4 I;
    recurse( scene->mRootNode, I, node2transformation );
    print( node2transformation );
    
    /// 1 Find the immediate parent of each bone
    
    
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
        save_rig( scene->mMeshes[ mesh_index ] );
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
