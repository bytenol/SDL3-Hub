#ifndef __BYTENOL_PAINTER2_BUFFER_HPP__
#define __BYTENOL_PAINTER2_BUFFER_HPP__


#include <iostream>
#include <vector>
#include "../../deps/glad/include/glad/glad.h"

namespace pnt
{

    enum BufferType
    {
        DEFAULT,
        SDF_BUFFER,
        BATCHED_BUFFER,
    };

    class BufferData
    {
    public:
        unsigned int vao, vbo;
        std::vector<float> data;
        int column;
                unsigned int instancedVbo;
        BufferType type = BufferType::DEFAULT;
        BufferData() = default;
        virtual void bind() = 0;
        virtual void resize(const int& capacity, const int& column) = 0;
        void push_back(const float& t) {
            data.push_back(t);
        }
        void clear();
        // void pushData(const float& d);
        virtual ~BufferData() = default;

    protected:

    };


    class BatchedBuffer: public BufferData
    {
    private:

    public:
        BatchedBuffer() {
            type = BufferType::BATCHED_BUFFER;
        };
        void resize(const int& capacity, const int& column) override;
        void bind() override;
    };


    class SdfBuffer: public BufferData
    {
    public:
        SdfBuffer(): BufferData() {
            type = BufferType::SDF_BUFFER;
        }
        void resize(const int& capacity, const int& column) override;
        void bind() override;
    };
}

#endif 