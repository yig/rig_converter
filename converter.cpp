// c++ -std=c++11 converter.cpp -o converter -I/usr/local/include -L/usr/local/lib -lassimp -g -Wall
/// When debugging, link against my assimp library:
// c++ -std=c++11 converter.cpp -o converter -I/usr/local/include -L/Users/yotam/Work/ext/assimp/build/code -lassimpd -g -Wall

#define SAVE_RIG 1

#include <iostream>
#include <iomanip> // setw, setprecision
#include <fstream>
#include <algorithm> // pair
#include <unordered_map>
#include <unordered_set>
#include <vector>
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

}

#if SAVE_RIG
namespace
{
typedef std::unordered_map< std::string, std::string > StringToString;
typedef std::unordered_map< std::string, aiVector3D > StringToVector3D;

void recurse( const aiNode* node, const aiMatrix4x4& parent_transformation, StringToVector3D& name2position, StringToString& name2parent ) {
    assert( node );
    const std::string name( node->mName.C_Str() );
    
    // We shouldn't have yet seen the node in any of our output maps.
    assert( name2position.find( name ) == name2position.end() );
    assert( name2parent.find( name ) == name2parent.end() );
    
    const aiMatrix4x4 transformation_so_far = parent_transformation * node->mTransformation;
    
    name2position[ name ] = aiVector3D( transformation_so_far.a4, transformation_so_far.b4, transformation_so_far.c4 );
    // The root node will not have a parent.
    if( nullptr != node->mParent ) name2parent[ name ] = node->mParent->mName.C_Str();
    
    for( int child_index = 0; child_index < node->mNumChildren; ++child_index ) {
        recurse( node->mChildren[ child_index ], transformation_so_far, name2position, name2parent );
    }
}
}

void save_rig( const aiScene* scene, std::vector< aiVector3D >& joints_out, std::vector< std::pair< int, int > >& bones_out, std::vector< float >& weights_out )
{
    /*
    Given an `aiScene*`, fills the output parameters:
        `joints_out` with all used joints,
        `bones_out` with pairs of indices (start,end) into `joints_out` corresponding
            to the start and end of each bone,
        `weights_out` with an array of weights for each vertex in scene.meshes (flattened)
            and for each bone in bones: [
                bone[0]-weight-for-vertex[0]
                bone[0]-weight-for-vertex[1]
                bone[0]-weight-for-vertex[2]
                ...
                bone[1]-weight-for-vertex[0]
                bone[1]-weight-for-vertex[1]
                bone[1]-weight-for-vertex[2]
                ...
                ]
    */
    
    printf( "# Extracting the rig.\n" );
    
    assert( scene );
    assert( scene->mRootNode );
    // This function doesn't make sense if there aren't any meshes.
    if( 0 == scene->mNumMeshes ) {
        std::cerr << "save_rig(): No meshes means no rig to save." << std::endl;
        return;
    }
    
    
    /// 1. Flatten the meshes into a single mesh. Store the index of the first vertex of
    ///    each mesh (assuming they are stored one after the other; if they're not,
    ///    I'll extract them myself).
    /// 2. Recursively traverse from the rootnode and (a) use the transformation matrices
    ///    to get the position of each joint and (b) store the parent of each joint.
    ///    These will be maps from joint name to matrix and from joint name to joint
    ///    parent's name.
    /// 3. Collect the set of all used bones among all meshes. Mesh bones store the name
    ///    of the joint on the "end" side of the directed edge. Also collect the set of
    ///    all used joints (the end joints and the parents of the end joints).
    /// 4. Save the skeleton bones in a simple format. Save all used joints' positions.
    ///    Save each bone as pairs of indices (start,end) into all used joints.
    ///    (libigl has a text file format called TGF that stores this: http://libigl.github.io/libigl/file-formats/ )
    /// 5. Save the skeleton weight matrix. Each mesh bone stores a sparse map from
    ///    vertex indices to weights for a small number of bones (as child joint names).
    ///    (I'll save it as a text file compatible with libigl's DMAT: http://libigl.github.io/libigl/file-formats/ )
    
    
    /// 1
    // Flattening is taken care of by ASSIMP saving to OBJ.
    std::vector< int > first_vertex_offsets;
    int total_vertex_num = 0;
    first_vertex_offsets.resize( scene->mNumMeshes );
    // The constructor for ints should set them to zero.
    assert( 0 == scene->mNumMeshes || 0 == first_vertex_offsets.at(0) );
    // Each mesh's first vertex comes immediately after the previous mesh's last vertex.
    // If the mesh's have N, M, and P vertices, then the offsets would be: [ 0, N, N+M ].
    total_vertex_num = 0;
    for( int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index ) {
        first_vertex_offsets.at( mesh_index ) = total_vertex_num;
        total_vertex_num += scene->mMeshes[ mesh_index ]->mNumVertices;
    }
    
    
    /// 2
    StringToString name2parent;
    StringToVector3D name2position;
    const aiMatrix4x4 I;
    recurse( scene->mRootNode, I, name2position, name2parent );
    // std::cout << "## name2transformation\n";
    // print( name2transformation );
    
    
    /// 3
    typedef std::unordered_set< std::string > StringSet;
    typedef std::vector< std::string > StringVector;
    
    StringSet used_bones, used_joints;
    for( int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index ) {
        const aiMesh* mesh = scene->mMeshes[ mesh_index ];
        assert( mesh );
        
        // Iterate over the bones of the mesh.
        for( int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index ) {
            const aiBone* bone = mesh->mBones[ bone_index ];
            
            const std::string bone_end_joint_name = bone->mName.C_Str();
            used_bones.insert( bone_end_joint_name );
            used_joints.insert( bone_end_joint_name );
            // Bones store the end joint name.
            // The bone must have a start joint that is the end joint's parent.
            assert( name2parent.find( bone_end_joint_name ) != name2parent.end() );
            used_joints.insert( name2parent[ bone_end_joint_name ] );
        }
    }
    const StringVector ordered_bones( used_bones.begin(), used_bones.end() );
    const StringVector ordered_joints( used_joints.begin(), used_joints.end() );
    
    
    /// 4
    // Save joints_out.
    joints_out.resize( ordered_joints.size() );
    // We need a reverse map (index to joint name) for saving bones_out.
    std::unordered_map< std::string, int > joint_name_to_ordered_joints_index;
    {
        int i = 0;
        for( const auto& name : ordered_joints ) {
            joints_out.at(i) = name2position[ name ];
            joint_name_to_ordered_joints_index[ name ] = i;
            ++i;
        }
    }
    
    // Save bones_out.
    bones_out.resize( ordered_bones.size() );
    {
        int i = 0;
        for( const auto& name : ordered_bones ) {
            bones_out.at(i) = std::make_pair(
                // start is parent
                joint_name_to_ordered_joints_index[ name2parent[ name ] ],
                // end is child
                joint_name_to_ordered_joints_index[ name ]
                );
            ++i;
        }
    }
    
    
    /// 5
    weights_out.assign( total_vertex_num * bones_out.size(), 0.f );
    for( int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index ) {
        const aiMesh* mesh = scene->mMeshes[ mesh_index ];
        assert( mesh );
        
        const int first_vertex_offset = first_vertex_offsets.at( mesh_index );
        
        // Iterate over the bones of the mesh.
        for( int bone_index = 0; bone_index < mesh->mNumBones; ++bone_index ) {
            const aiBone* bone = mesh->mBones[ bone_index ];
            assert( bone );
            
            // Iterate over the corresponding vertex weights of the bone.
            for( int weight_index = 0; weight_index < bone->mNumWeights; ++weight_index ) {
                const aiVertexWeight& weight = bone->mWeights[ weight_index ];
                
                const int local_vertex_index = weight.mVertexId;
                assert( local_vertex_index >= 0 );
                assert( local_vertex_index < mesh->mNumVertices );
                assert( local_vertex_index + first_vertex_offset < total_vertex_num );
                
                weights_out.at( bone_index*total_vertex_num + first_vertex_offset+local_vertex_index ) = weight.mWeight;
            }
        }
    }
}

void save_skeleton_to_TGF( const std::string& filename, const std::vector< aiVector3D >& joints, const std::vector< std::pair< int, int > >& bones ) {
    /*
    Saves the given `joints` (positions) and `bones` (pairs of (start,end) indices into joints)
    to the file named `filename` in TGF format.
    
    TGF format: http://libigl.github.io/libigl/file-formats/tgf.html
    */
    
    std::ofstream out( filename );
    if( !out ) {
        std::cerr << "save_skeleton_to_TGF(): Unable to open file for writing: " << filename << std::endl;
        return;
    }
    
    {
        // TGF is 1-indexed.
        int i = 1;
        for( const auto& position : joints ) {
            // Each line: index x y z
            out
                << std::setw( 4 ) << i << ' '
                // Q: What should setprecision be to not lose any accuracy when printing a double?
                // A: 17. See: http://stackoverflow.com/questions/554063/how-do-i-print-a-double-value-with-full-precision-using-cout
                << std::setw( 27 ) << std::setprecision( 17 ) << position.x << ' '
                << std::setw( 27 ) << std::setprecision( 17 ) << position.y << ' '
                << std::setw( 27 ) << std::setprecision( 17 ) << position.z << std::endl;
            ++i;
        }
    }
    
    out << "#" << std::endl;
    
    for( const auto& bone : bones ) {
        // Each line: start_index end_index is_bone is_pseudo_edge is_cage
        // TGF is 1-indexed.
        // The "1 0 0" means that this is a bone edge, and not another kind of edge.
        out << std::setw( 4 ) << (bone.first + 1) << ' ' << std::setw( 4 ) << (bone.second + 1) << " 1 0 0" << std::endl;
    }
}

void save_weights_to_DMAT( const std::string& filename, int rows, int cols, const std::vector< float >& weights ) {
    /*
    Saves the given `weights` (column-major matrix) with `rows` and `cols` dimensions
    to the file named `filename` in DMAT format.
    
    DMAT format: http://libigl.github.io/libigl/file-formats/dmat.html
    */
    
    assert( rows > 0 );
    assert( cols > 0 );
    assert( rows * cols == weights.size() );
    
    std::ofstream out( filename );
    if( !out ) {
        std::cerr << "save_weights_to_DMAT(): Unable to open file for writing: " << filename << std::endl;
        return;
    }
    
    // Save the cols and rows.
    out << cols << ' ' << rows << std::endl;
    
    for( const auto& val : weights ) {
        // Q: What should setprecision be to not lose any accuracy when printing a double?
        // A: 17. See: http://stackoverflow.com/questions/554063/how-do-i-print-a-double-value-with-full-precision-using-cout
        out << std::setprecision( 17 ) << val << std::endl;
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
    {
        std::vector< aiVector3D > joints_out;
        std::vector< std::pair< int, int > > bones_out;
        std::vector< float > weights_out;
        save_rig( scene, joints_out, bones_out, weights_out );
        assert( weights_out.size() % bones_out.size() == 0 );
        
        std::string filename = os_path_splitext( outpath ).first + ".tgf";
        save_skeleton_to_TGF( filename, joints_out, bones_out );
        std::cout << "Saved: " << filename << std::endl;
        
        filename = os_path_splitext( outpath ).first + ".dmat";
        save_weights_to_DMAT( filename, weights_out.size() / bones_out.size(), bones_out.size(), weights_out );
        std::cout << "Saved: " << filename << std::endl;
    }
#endif
    
    // Cleanup.
    aiReleaseImport( scene );
    
    return 0;
}
