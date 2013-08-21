/*
 * Copyright 2013, Blender Foundation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "openvdb_volume.h"
#include <openvdb/openvdb.h>

CCL_NAMESPACE_BEGIN

typedef struct VDBVolumeFile {    
	openvdb::io::File file;
    ustring version;
    
	openvdb::GridPtrVecPtr grids;
    openvdb::MetaMap::Ptr meta;

    VDBVolumeFile(ustring filename) : file(filename.string())
    {
        file.open();
        
        grids = file.getGrids();
        meta = file.getMetadata();
        version = file.version();
    }
    ~VDBVolumeFile()
    {
        file.close();
    }
    
} VDBVolumeFile;

class OpenVDBUtil
{
public:
    static ustring u_openvdb_file_extension;
    
	static void initialize_library();
	static bool open_file(OIIO::ustring filename, VDBVolumeFile *vdb_volume);
	static bool is_vdb_volume_file(OIIO::ustring filename);
    static VDBVolumeFile *get_volume_from_file(OIIO::ustring filename);
    static int get_number_of_grids(VDBVolumeFile vdb_volume);
    static int nearest_neighbor(float worldCoord);
    
private:
    static bool vdb_file_check_extension(ustring filename);
    static bool vdb_file_check_valid_header(ustring filename);
};


/* static ustrings */
ustring OpenVDBUtil::u_openvdb_file_extension(".vdb");


void OpenVDBUtil::initialize_library()
{
	openvdb::initialize();
}

bool OpenVDBUtil::vdb_file_check_valid_header(ustring filename)
{
    OpenVDBUtil::initialize_library();
    openvdb::io::File file(filename.string());
    file.open();
    
    OIIO::ustring openvdb_version;
    
    openvdb_version.empty();
    openvdb_version = file.version();
    
    file.close();
    
    if(openvdb_version.length() > 0)
        return true;
    
	return false;
}

bool OpenVDBUtil::vdb_file_check_extension(ustring filename)
{
    if (filename.substr(filename.length() - u_openvdb_file_extension.length(),
                        u_openvdb_file_extension.length()) == u_openvdb_file_extension)
		return true;
    
    return false;
}


bool OpenVDBUtil::is_vdb_volume_file(OIIO::ustring filename)
{
    OIIO::ustring openvdb_version;
    
    if (vdb_file_check_extension(filename)) {
        if (vdb_file_check_valid_header(filename))
            return true;
    }
    
    return false;
}


int OpenVDBUtil::get_number_of_grids(VDBVolumeFile vdb_volume)
{
    return vdb_volume.grids->size();
}

int OpenVDBUtil::nearest_neighbor(float worldCoord)
{
    int x = static_cast<int>(floor(worldCoord + 0.5));
    return x;
}


// VDBTextureSystem

VDBTextureSystem::Ptr VDBTextureSystem::init() {
    OpenVDBUtil::initialize_library();
    
    Ptr vdb_ts(new VDBTextureSystem());
    
    return vdb_ts;
}

bool VDBTextureSystem::is_vdb_volume(ustring filename)
{
    if (vdb_files.empty())
    {
        return OpenVDBUtil::is_vdb_volume_file(filename);
    }
    if (vdb_files.find(filename) != vdb_files.end())
        return true;
    else
        return OpenVDBUtil::is_vdb_volume_file(filename);
}

bool VDBTextureSystem::perform_lookup(ustring filename, TextureOpt &options, OSL::ShaderGlobals *sg,
                                      const Imath::V3f &P, const Imath::V3f &dPdx,
                                      const Imath::V3f &dPdy, const Imath::V3f &dPdz,
                                      float *result)
{
    // - check if file is open;
    VDBMap::const_iterator open_file = vdb_files.find(filename);
    
    if (open_file == vdb_files.end())
    {
        open_file = add_vdb_to_map(filename);
    }

    VDBFilePtr vdb_p = open_file->second;
    
    //decide on type of grid; let's say it's a float grid.
    openvdb::GridPtrVec::const_iterator it = vdb_p->grids->begin();
    const openvdb::GridBase::Ptr grid = *it;
    
    openvdb::FloatGrid::Ptr floatGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(grid);
    
    openvdb::FloatGrid::Accessor accessor = floatGrid->getAccessor();
    
    float x, y, z = 0;
    x = OpenVDBUtil::nearest_neighbor(P[0]);
    y = OpenVDBUtil::nearest_neighbor(P[1]);
    z = OpenVDBUtil::nearest_neighbor(P[2]);
    
    openvdb::Coord point((int)x, (int)y, (int)z);
    
    const float myResult(accessor.getValue(point));
    *result = myResult; 
    return true;
}

VDBTextureSystem::VDBMap::const_iterator VDBTextureSystem::add_vdb_to_map(ustring filename)
{
    VDBFilePtr vdb_sp(new VDBVolumeFile(filename));
    
    return (vdb_files.insert(std::make_pair(filename, vdb_sp))).first;
}

void VDBTextureSystem::free(VDBTextureSystem::Ptr vdb_ts)
{
    vdb_ts.reset();
}

VDBTextureSystem::~VDBTextureSystem(){
}

CCL_NAMESPACE_END
