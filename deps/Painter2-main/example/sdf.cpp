#include <iostream>
#include <string>
#include <vector>

#include <painter2/core.hpp>

using namespace pnt;

class Window: public Painter2
{
    float angle = 0.0f;

public:
    Window(const std::string& t, const int& w, const int& h): Painter2(t, w, h){}

protected:
    bool onReady() override
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        
        return true;
    }

    bool onRender() override
    {
        angle++;

        // beginUseBuffer(SDF_BUFFER);
        save();
        setTranslation(125, 125);
        setRotation(angle * 3.14159f / 180);
        setRenderColor(1.0f, 0.0f, 0.0f);
        renderShape(-50, -50, 50, 50, ShapeType::Rect, RenderMode::Fill);
        setRenderLineWidth(0.01f);
        setRenderColor(0, 1, 0);
        renderShape(-25, -25, 50, 50, ShapeType::Rect, RenderMode::Stroke);
        restore();
        renderShape(20, 100, 50, 50, ShapeType::Circle, RenderMode::Fill);

        // endUseBuffer();

        // setRenderColor(0.65f, 0.82f, 0.34f);

        // beginUseBuffer(BATCHED_BUFFER);
        // const float x = 200, y = 200, w = 100, h = 100;
        // save();
        // setTranslation(x + w*0.5f, y + h*0.5f);
        // setRotation(angle * 3.14159f / 180.0f);
        // setScale(2, 2);
        // setRenderColor(1.0f, 0.0f, 1.0f);
        // renderFillRect(-w*0.5f, -h*0.5f, w, h);
        // restore();

        // setRenderLineWidth(2.0f);
        // setRenderColor(0.0f, 0.0f, 1.0f);
        // renderStrokeRect(x, y, w, h);

        // setRenderLineWidth(1.0f);
        // setRenderColor(0.3, 0.6, 0.4);
        // renderLine(20.0f, 20.0f, 200.0f, 100.0f);

        // setRenderColor(0.65f, 0.82f, 0.34f);
        // renderFillArc(100.0f, 100.0f, 25.0f);   

        // std::vector<VertexData> triangle;
        // triangle.push_back({ {0, -50}, { 0.234f, 0.632f, 0.123f, 1.0f } });
        // triangle.push_back({ {50, 50}, { 0.534f, 0.232f, 0.123f, 1.0f } });
        // triangle.push_back({ {-50, 50} });

        // setRenderColor(1.0f, 0.0f, 0.0f);
        // renderFillPolygon(150, 150, triangle);

        // setRenderLineWidth(3.0f);
        // setRenderColor(0.5f, 0.2f, 0.45f);
        // renderStrokePolygon(150, 150, triangle);
        // endUseBuffer();

        return true;
    }
};



int main(int argc, char const *argv[])
{
    Window w{"SDF", 620, 480};
    if(!w.start()) {
        std::cout << w.getError() << std::endl;
        return -1;
    }
    return 0;
}
