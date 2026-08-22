#include "openminecraft/renderer/common/model/om_renderer_model_obj.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <istream>
#include "tiny_obj_loader.h"

namespace openminecraft::renderer::common::model
{
OMRendererModelObj::OMRendererModelObj(OMRenderer *renderer, std::istream *src)
{
    format.appendPart("inPosition", basics::Vec3f)
        ->appendPart("inTexCoord", basics::Vec2f)
        ->appendPart("inNormal", basics::Vec3f)
        ->nextGroup()
        ->decideStruct();

    std::vector<Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, src);

        std::unordered_map<Vertex, uint32_t> uniqueVertices;

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                Vertex v;

                v.get<0>().x = attrib.vertices[3 * index.vertex_index + 0];
                v.get<0>().y = attrib.vertices[3 * index.vertex_index + 1];
                v.get<0>().z = attrib.vertices[3 * index.vertex_index + 2];

                if (index.texcoord_index >= 0)
                {
                    v.get<1>().x = attrib.texcoords[2 * index.texcoord_index + 0];
                    v.get<1>().y = attrib.texcoords[2 * index.texcoord_index + 1];
                }
                else
                {
                    v.get<1>() = glm::vec2(0.0f, 0.0f);
                }

                v.get<2>().x = attrib.normals[3 * index.normal_index + 0];
                v.get<2>().y = attrib.normals[3 * index.normal_index + 1];
                v.get<2>().z = attrib.normals[3 * index.normal_index + 2];

                auto it = uniqueVertices.find(v);
                if (it != uniqueVertices.end())
                {
                    indices.push_back(it->second);
                }
                else
                {
                    auto newIndex = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(v);
                    uniqueVertices[v] = newIndex;
                    indices.push_back(newIndex);
                }
            }
        }
    }

    auto siz = vertices.size() * sizeof(Vertex);

    vertexData = renderer->allocateBuffer(VertexData, siz);
    vertexData->updateData(vertices.data());

    siz = indices.size() * sizeof(uint32_t);
    vertexIndex = renderer->allocateBuffer(VertexIndex, siz);
    vertexIndex->updateData(indices.data());

    vertexCount = indices.size();
}

OMRendererModelObj::~OMRendererModelObj()
{
    delete vertexData;
    delete vertexIndex;
}
} // namespace openminecraft::renderer::common::model
