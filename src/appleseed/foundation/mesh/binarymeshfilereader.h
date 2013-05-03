
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef APPLESEED_FOUNDATION_MESH_BINARYMESHFILEREADER_H
#define APPLESEED_FOUNDATION_MESH_BINARYMESHFILEREADER_H

// appleseed.foundation headers.
#include "foundation/mesh/imeshfilereader.h"
#include "foundation/platform/compiler.h"

// Standard headers.
#include <cstddef>
#include <string>
#include <vector>

// Forward declarations.
namespace foundation    { class IMeshBuilder; }

namespace foundation
{

//
// Read for a simple binary mesh file format.
//

class BinaryMeshFileReader
  : public IMeshFileReader
{
  public:
    // Constructor.
    explicit BinaryMeshFileReader(const std::string& filename);

    // Read a mesh.
    virtual void read(IMeshBuilder& builder) OVERRIDE;

  private:
    const std::string       m_filename;
    std::vector<size_t>     m_vertices;
    std::vector<size_t>     m_vertex_normals;
    std::vector<size_t>     m_tex_coords;

    std::string read_string(std::FILE* file);

    bool read_mesh(IMeshBuilder& builder, std::FILE* file);
    void read_vertices(IMeshBuilder& builder, std::FILE* file);
    void read_vertex_normals(IMeshBuilder& builder, std::FILE* file);
    void read_texture_coordinates(IMeshBuilder& builder, std::FILE* file);
    void read_material_slots(IMeshBuilder& builder, std::FILE* file);
    void read_faces(IMeshBuilder& builder, std::FILE* file);
    void read_face(IMeshBuilder& builder, std::FILE* file);
};

}       // namespace foundation

#endif  // !APPLESEED_FOUNDATION_MESH_BINARYMESHFILEREADER_H