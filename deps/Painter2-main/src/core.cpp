#include <painter2/core.hpp>


pnt::Painter2::Painter2(const std::string &t, const int &w, const int &h)
{
    title = t;
    width = w;
    height = h;
}


bool pnt::Painter2::start()
{
    if(!initGLFW()) return false;

    compileBasicShader(sdfVertexShaderSrc, sdfFragmentShaderSrc, sdfShaderProgram);
    compileBasicShader(vertexShaderSrc, fragmentShaderSrc, basicShaderProgram);

    if(!onReady()) return false;
    
    int mProj1, mProj2;
    mProj1 = glGetUniformLocation(basicShaderProgram, "projectionMatrix");
    mProj2 = glGetUniformLocation(sdfShaderProgram, "projectionMatrix");

    // glUseProgram(basicShaderProgram);
    auto projMat = glm::ortho<float>(0.0f, width, height, 0.0f, 1.0f, -1.0f);
    glUseProgram(basicShaderProgram);
    glUniformMatrix4fv(mProj1, 1, GL_FALSE, glm::value_ptr(projMat));

    glUseProgram(sdfShaderProgram);
    glUniformMatrix4fv(mProj2, 1, GL_FALSE, glm::value_ptr(projMat));

    glViewport(0, 0, width, height);

    // setup buffers
    // reset buffers for use
    preComputeGeometry();
    modelMatrices.push(glm::mat4(1.0f));

    basicShapeBuffer.resize(200000, 6);
    sdfShapeBuffer.resize(500000, 12);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);

        beginUseBuffer(SDF_BUFFER);
        onRender();
        endUseBuffer();
        glfwSwapBuffers(window);

        // This makes it safe to use save() function without calling restore()
        // otherwise, the modelMatrices will get larger as matrices keep added every frame
        while(modelMatrices.size() > 1) modelMatrices.pop();
    }

    return true;
}

const std::string &pnt::Painter2::getError() const
{
    return error;
}

pnt::Painter2::~Painter2()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}


const int &pnt::Painter2::getWidth() const
{
    return width;
}

const int &pnt::Painter2::getHeight() const
{
    return height;
}

void pnt::Painter2::setError(const std::string &msg)
{
    error = msg;
}


void pnt::Painter2::beginUseBuffer(const BufferType& type)
{
    switch (type)
    {
    case SDF_BUFFER:
        currentBuffer = &sdfShapeBuffer;
        glUseProgram(sdfShaderProgram);
        break;
    case BATCHED_BUFFER:
        currentBuffer = &basicShapeBuffer;
        glUseProgram(basicShaderProgram);
    default:
        break;
    }
    currentBuffer->clear();
}


void pnt::Painter2::beginUseBuffer(BufferData *buffer)
{
    if(!buffer) {
        currentBuffer = &sdfShapeBuffer;
    } else
    currentBuffer = buffer;

    currentBuffer->clear();
}



void pnt::Painter2::endUseBuffer()
{
    if(!currentBuffer) return;
    currentBuffer->bind();
    currentBuffer = nullptr;
}


void pnt::Painter2::setRenderColor(const float& r, const float& g, const float& b, const float& a)
{
    fillColor.x = r;
    fillColor.y = g;
    fillColor.z = b;
    fillColor.w = a;
}

void pnt::Painter2::setRenderLineWidth(const float &width)
{
    strokeWidth = width;
}


void pnt::Painter2::renderShape(const float& x, const float& y, const float& w, const float& h, const ShapeType& shape, const RenderMode& mode)
{
    if(!(typeid(*currentBuffer) == typeid(SdfBuffer)))
        throw std::logic_error("Current Buffer must be an instance of SdfBuffer");
    pushVertex(x, y, w, h, shape, mode);
}


void pnt::Painter2::renderFillRect(const float &x, const float &y, const float &w, const float &h)
{   
    glm::vec2 p0{ x, y },
    p1{ x + w, y },
    p2{ x + w, y + h },
    p3{ x, y + h };

    pushVertex(p0); pushVertex(p1); pushVertex(p2); 
    pushVertex(p0); pushVertex(p2); pushVertex(p3);
}


void pnt::Painter2::renderStrokeRect(const float &x, const float &y, const float &w, const float &h)
{
    renderFillRect(x, y, w, strokeWidth);
    renderFillRect(x, y + h, w, strokeWidth);
    renderFillRect(x, y, strokeWidth, h);
    renderFillRect(x + w, y, strokeWidth, h);
}

void pnt::Painter2::renderLine(const float &x1, const float &y1, const float &x2, const float &y2)
{
    glm::vec2 p0{ x1, y1 };
    glm::vec2 p1{ x2, y2 };

    auto dir = glm::normalize(p1 - p0);
    auto normal = glm::vec2(-dir.y, dir.x);
    auto offset = normal * (strokeWidth * 0.5f);

    glm::vec2 v0 = p0 + offset;
    glm::vec2 v1 = p1 + offset;
    glm::vec2 v2 = p1 - offset;
    glm::vec2 v3 = p0 - offset;

    pushVertex(v0); pushVertex(v1); pushVertex(v2); 
    pushVertex(v0); pushVertex(v2); pushVertex(v3);
}


void pnt::Painter2::renderFillArc(const float& x, const float& y, const float& r, const float& startAngle, const float& endAngle)
{
    glm::vec2 origin{ x, y };
    glm::vec2 prevPos{ x + r, y};

    for(int i = 0; i < unitCircleData.size(); i+=2)
    {
        auto pos = origin + glm::vec2{ unitCircleData[i] * r, unitCircleData[i+1] * r };
        pushVertex(origin);
        pushVertex(prevPos);
        pushVertex(pos);
        prevPos = pos;
    }
}


void pnt::Painter2::renderFillPolygon(const float& x, const float& y, const std::vector<VertexData>& data)
{
    glm::vec2 origin{ x, y };

    for(int i = 0; i < data.size(); i++)
    {
        bool useDefaultFill = data[i].color.x < 0 || data[i].color.y < 0 || data[i].color.z < 0;
        auto curr = origin + data[i].pos;
        auto next = origin + data[(i + 1) % data.size()].pos;
        auto lastFill = fillColor;
        if(!useDefaultFill) fillColor = data[i].color;
        pushVertex(origin);
        pushVertex(curr);
        pushVertex(next);
        fillColor = lastFill;
    }
}



void pnt::Painter2::renderStrokePolygon(const float& x, const float& y, const std::vector<VertexData>& data)
{
    glm::vec2 origin{ x, y };

    for(int i = 0; i < data.size(); i++)
    {
        bool useDefaultFill = data[i].color.x < 0 || data[i].color.y < 0 || data[i].color.z < 0;
        auto curr = origin + data[i].pos;
        auto next = origin + data[(i + 1) % data.size()].pos;
        auto lastFill = fillColor;
        if(!useDefaultFill) fillColor = data[i].color;
        renderLine(curr.x, curr.y, next.x, next.y);
        fillColor = lastFill;
    }
}



void pnt::Painter2::save(glm::mat4* m)
{
    if(!m) modelMatrices.push(glm::mat4(1.0f));
    else modelMatrices.push(*m);
}

void pnt::Painter2::restore()
{
    if(modelMatrices.size() <= 1) return;
    modelMatrices.pop();
}

void pnt::Painter2::setTranslation(const float &x, const float &y)
{
    modelMatrices.top() = glm::translate(modelMatrices.top(), glm::vec3(x, y, 0.0f));
}

void pnt::Painter2::setRotation(const float &angleInRadians)
{
    modelMatrices.top() = glm::rotate(modelMatrices.top(), angleInRadians, glm::vec3(0, 0, 1));
}

void pnt::Painter2::setScale(const float &sx, const float &sy)
{
    modelMatrices.top() = glm::scale(modelMatrices.top(), glm::vec3(sx, sy, 0));
}


void pnt::Painter2::pushVertex(const glm::vec2& pos)
{
    auto tPos = modelMatrices.top() * glm::vec4(pos, 0, 1.0f);
    currentBuffer->push_back(tPos.x);
    currentBuffer->push_back(tPos.y);
    // color
    currentBuffer->push_back(fillColor.x);
    currentBuffer->push_back(fillColor.y);
    currentBuffer->push_back(fillColor.z);
    currentBuffer->push_back(fillColor.w);
}


void pnt::Painter2::pushVertex(const float& x, const float& y, const float& w, const float& h, const ShapeType& shape, const RenderMode& mode)
{
    auto tPos = modelMatrices.top() * glm::vec4(glm::vec2(x, y), 0, 1.0f);
    currentBuffer->push_back(tPos.x);
    currentBuffer->push_back(tPos.y);

    // size
    currentBuffer->push_back(w);
    currentBuffer->push_back(h);

    // color
    currentBuffer->push_back(fillColor.x);
    currentBuffer->push_back(fillColor.y);
    currentBuffer->push_back(fillColor.z);
    currentBuffer->push_back(fillColor.w);

    // mode
    currentBuffer->push_back(shape); // shape
    currentBuffer->push_back(mode != Fill); // mode [fill/stroke]
    currentBuffer->push_back(strokeWidth); // strokeWidth
    currentBuffer->push_back(strokeRadius); // radius
}


void pnt::Painter2::preComputeGeometry()
{
    // unit circle
    int segments = 10;

    unitCircleData.clear();
    for(int i = 0; i <= 360; i+=segments)
    {
        auto angle = glm::radians((float)i);
        unitCircleData.push_back(std::cos(angle));
        unitCircleData.push_back(std::sin(angle));
    }
}


bool pnt::Painter2::initGLFW()
{
    if(!glfwInit()) {
        setError("Unable to initialize glfw");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (!window) {
        setError("Unable to create window context for opengl4.4");
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        setError("Unable to load GLAD.c");
        return false;
    }

    return true;
}


void pnt::Painter2::compileBasicShader(const char* vShader, const char* fShader, unsigned int& program)
{

    char info[512];

    auto compile = [&info](GLenum type, const char* src) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(shader, 512, nullptr, info);
            std::cerr << "Shader Compile Error: " << info << std::endl;
        }
        return shader;
    };

    unsigned int vs = compile(GL_VERTEX_SHADER, vShader);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, fShader);    

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
}