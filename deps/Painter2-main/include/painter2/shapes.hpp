#ifndef __BYTENOL_PAINTER2_SHAPES_HPP__
#define __BYTENOL_PAINTER2_SHAPES_HPP__

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "buffer.hpp"


namespace pnt
{
    

    enum ShapeType {
        Circle = 0,
        Rect,
        RoundedRect,
        Ellipse,
        Capsule,
        Polygon,
        Arc
    };  

    enum RenderMode {
        Fill = 0,
        Stroke = 1
    };

    struct VertexData
    {   
        glm::vec2 pos;
        glm::vec4 color { -1, -1, -1, 1 };
    };
    
}

#endif 
