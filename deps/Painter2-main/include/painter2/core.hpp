#ifndef __BYTENOL_PAINTER2_CORE_HPP__
#define __BYTENOL_PAINTER2_CORE_HPP__

#include <iostream>
#include <string>
#include <chrono>
#include <stack>

#include "../../deps/glad/include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "buffer.hpp"
#include "shapes.hpp"

namespace pnt
{

    static const char* sdfVertexShaderSrc = R"(
        #version 330 core

        layout(location = 0) in vec2 aLocalPos;        // quad vertex (-0.5 to 0.5)

        layout(location = 1) in vec2 aInstancePos;     // per instance
        layout(location = 2) in vec2 aInstanceSize;
        layout(location = 3) in vec4 aInstanceColor;
        layout (location = 4) in vec4 aRenderParams; // vShape, vRenderMode, vStrokeWidth, vRadius

        uniform mat4 projectionMatrix;

        out vec2 vLocalPos;
        out vec2 vSize;
        out vec4 vColor;

        flat out float vStrokeWidth;
        flat out float vRadius;
        flat out int vRenderMode;
        flat out int vShape;


        void main()
        {
            vec2 scaled = aLocalPos * aInstanceSize;
            vec2 worldPos = scaled + aInstancePos;
            gl_Position = projectionMatrix * vec4(worldPos, 0.0, 1.0);

            vLocalPos = scaled;        // centered coordinates
            vSize = aInstanceSize;
            vColor = aInstanceColor;
            vShape = int(aRenderParams.x);
            vRenderMode = int(aRenderParams.y);
            vStrokeWidth = aRenderParams.z;
            vRadius = aRenderParams.w;
        }
    )";

    static const char* sdfFragmentShaderSrc = R"(
    #version 330 core

    in vec2 vLocalPos;     // centered coords
    in vec2 vSize;
    in vec4 vColor;

    flat in int vRenderMode;
    flat in float vStrokeWidth;
    flat in float vRadius;
    flat in int vShape;

    out vec4 FragColor;

    const float AA = 1.0;

    // --------------------
    // SDF SHAPES
    // --------------------

    float sdCircle(vec2 p, float r)
    {
        return length(p) - r;
    }

    float sdBox(vec2 p, vec2 b)
    {
        vec2 d = abs(p) - b;
        return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
    }

    float sdRoundedBox(vec2 p, vec2 b, float r)
    {
        vec2 q = abs(p) - b + r;
        return length(max(q,0.0)) - r;
    }

    float sdEllipse(vec2 p, vec2 r)
    {
        return (length(p / r) - 1.0) * min(r.x, r.y);
    }

    float sdCapsule(vec2 p, vec2 a, vec2 b, float r)
    {
        vec2 pa = p - a;
        vec2 ba = b - a;
        float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
        return length(pa - ba * h) - r;
    }

    float sdRegularPolygon(vec2 p, int sides, float radius)
    {
        float angle = atan(p.y, p.x);
        float sector = 2.0 * 3.1415926 / float(sides);
        float d = cos(floor(0.5 + angle/sector)*sector - angle) * length(p);
        return d - radius;
    }

    // float sdArc(vec2 p, float radius, float angle)
    // {
    //     float l = length(p);
    //     float a = atan(p.y, p.x);
    //     float half = angle * 0.5;
    //     float arcDist = max(abs(a) - half, 0.0) * radius;
    //     return max(abs(l - radius), arcDist);
    // }



    // --------------------
    // MAIN
    // --------------------

    void main()
    {
        vec2 halfSize = vSize * 0.5;
        float dist = 0.0;

        if (vShape == 0)          // Circle
            dist = sdCircle(vLocalPos, min(halfSize.x, halfSize.y));

        else if (vShape == 1)     // Rect
            dist = sdBox(vLocalPos, halfSize);

        else if (vShape == 2)     // Rounded Rect
            dist = sdRoundedBox(vLocalPos, halfSize, vRadius);

        else if (vShape == 3)     // Ellipse
            dist = sdEllipse(vLocalPos, halfSize);

        else if (vShape == 4)     // Capsule
            dist = sdCapsule(vLocalPos,
                            vec2(-halfSize.x + vRadius, 0.0),
                            vec2( halfSize.x - vRadius, 0.0),
                            vRadius);
        // else if (vShape == 5)     // Polygon
        //     dist = sdRegularPolygon(vLocalPos, int(vParam), min(halfSize.x, halfSize.y));

        float alpha = 0.0;

        alpha = 1.0 - smoothstep(0.0, AA, dist);

        if (vRenderMode == 0) // FILL
        {
            alpha = 1.0 - smoothstep(0.0, AA, dist);
        }
        else // STROKE
        {
            float d = abs(dist) - vStrokeWidth;
            alpha = 1.0 - smoothstep(0.0, AA, d);
        }

        if (alpha <= 0.0)
            discard;


        FragColor = vec4(vColor.rgb, vColor.a * alpha);
    }
    )";

    static const char* vertexShaderSrc = R"(
    #version 410 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec4 aColor;
    layout (location = 2) in mat4 mModel;

    uniform mat4 projectionMatrix;

    out vec4 vColor;

    void main() {
        gl_Position = projectionMatrix * vec4(aPos, 0.0, 1.0);
        vColor = aColor;
    }
    )";

    static const char* fragmentShaderSrc = R"(
    #version 410 core
    in vec4 vColor;
    out vec4 FragColor;
    void main() {
        FragColor = vColor;
    }
    )";


    


    class Painter2
    {
    private:
        int width, height;
        std::string title, error;
        GLFWwindow* window = nullptr;

        bool windowShouldClose = false;

        // shader programs
        unsigned int basicShaderProgram, sdfShaderProgram;


        glm::vec4 fillColor{ 0, 0, 0, 1 };
        float strokeWidth = 1.0f;
        float strokeRadius = 0.0f;

        // transformation matrices
        glm::mat4 mModel;
        std::stack<glm::mat4> modelMatrices;

        glm::vec2 modelScale;

        // geometry
        std::vector<float> unitCircleData;

        // default buffer
        BufferData* currentBuffer = nullptr;
        BatchedBuffer basicShapeBuffer;
        SdfBuffer sdfShapeBuffer;

    public:
        Painter2(const std::string& t, const int& w, const int& h);
        bool start();
        const std::string& getError() const;
        virtual ~Painter2();

    protected:
        virtual bool onReady() { return true; }
        virtual bool onRender() { return true; }

        const int& getWidth() const;
        const int& getHeight() const;
        void setError(const std::string& msg);

        void beginUseBuffer(const BufferType& type);
        void beginUseBuffer(BufferData* buffer = nullptr);
        void endUseBuffer();

        /// @brief set the per-shape color for rendering
        /// @param r is the red value from 0.0f to 1.0f
        /// @param g is the green value from 0.0f to 1.0f
        /// @param b is the blue value from 0.0f to 1.0f
        /// @param a is the alpha value from 0.0f to 1.0f
        void setRenderColor(const float& r, const float& g, const float& b, const float& a = 1.0f);


        /// @brief set the thickness for drawing lines and stroked shapes
        /// @param width is the thickness value
        void setRenderLineWidth(const float& width);


        void renderShape(const float& x, const float& y, const float& w, const float& h, const ShapeType& shape, const RenderMode& mode);


        /// @brief draw a filled rectangle 
        /// @param x is the x-position
        /// @param y is the y-position
        /// @param w is the width of the rectangle
        /// @param h is the height of the rectangle
        void renderFillRect(const float& x, const float& y, const float& w, const float& h);


        /// @brief draw a stroked rectangle
        /// @param x is the x-position
        /// @param y is the y-position
        /// @param w is the width of the rectangle
        /// @param h is the height of the rectangle
        void renderStrokeRect(const float& x, const float& y, const float& w, const float& h);


        /// @brief draw a line segment between two points
        /// @param x1 is the x positio
        /// @param y1 
        /// @param x2 
        /// @param y2 
        void renderLine(const float& x1, const float& y1, const float& x2, const float& y2);
        void renderFillArc(const float& x, const float& y, const float& r, const float& startAngle = 0.0f, const float& endAngle = 360.0f);
        void renderFillPolygon(const float& x, const float& y, const std::vector<VertexData>& data);
        void renderStrokePolygon(const float& x, const float& y, const std::vector<VertexData>& data);

        // matrix methods

        void save(glm::mat4* m = nullptr);
        void restore();
        void setTranslation(const float& x, const float& y);
        void setRotation(const float& angleInRadians);
        void setScale(const float& sx, const float& sy);

    private:
        void pushVertex(const glm::vec2& pos);
        /// @brief used in a batch renderer to push required batch data
        void pushVertex(const float& x, const float& y, const float& w, const float& h, const ShapeType& shape, const RenderMode& mode);


        void preComputeGeometry();


        /// initialize glfw window context and load glad
        bool initGLFW();


        /// @brief compile shader into programs
        /// @param vShader is the vertex shader
        /// @param fShader is the fragment shader
        /// @param program is a reference to a program
        void compileBasicShader(const char* vShader, const char* fShader, unsigned int& program);
    };
    

}

#endif 