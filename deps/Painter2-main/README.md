# Painter2
An opengl 4 based rendering engine

_____PUBLIC_____
Painter2(const std::string& t, const int& w, const int& h);
bool start();
const std::string& getError() const;

____PROTECTED_______

setRenderColor()
renderFillRect()
renderStrokeRect()
renderLine(20.0f, 20.0f, 200.0f, 100.0f);
renderFillArc(100.0f, 100.0f, 25.0f);
renderFillPolygon(150, 150, triangle);
renderStrokePolygon(150, 150, triangle);

save(nullptr);
setTranslation(x + w*0.5f, y + h*0.5f);
setRotation(angle * 3.14159f / 180.0f);
restore()

setScale(2, 2);

setRenderLineWidth(2.0f);

beginUseBuffer(nullptr)
endUseBuffer();

virtual bool onReady() { return true; }
virtual bool onRender() { return true; }

const int& getWidth() const;
const int& getHeight() const;
void setError(const std::string& msg);
