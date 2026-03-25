#include <painter2/buffer.hpp>

void pnt::BufferData::clear()
{
    data.clear();
}


/// @brief reset the buffer data. This function should only be called once
/// @param capacity is the maximum capacity of the buffer
/// @param column is the column size of the data array
void pnt::BatchedBuffer::resize(const int &capacity, const int &column)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    this->column = column;
    int _stride = sizeof(float) * column;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, _stride * capacity, nullptr, GL_DYNAMIC_DRAW);

    data.clear();
    data.reserve(column * capacity);

    // x, y, r, g, b, a

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, _stride, (void*)0); // position
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, _stride, (void*)(2 * sizeof(float))); // position
}


void pnt::BatchedBuffer::bind()
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(data.size()) / column);
}



void pnt::SdfBuffer::resize(const int &capacity, const int &column)
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &instancedVbo);

    std::vector<float> vData {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        -0.5f, 0.5f,
        0.5f, 0.5f
    };

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vData.size(), vData.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(0));

    int stride = sizeof(float) * column;
    data.clear();
    data.reserve(column * capacity);

    glBindBuffer(GL_ARRAY_BUFFER, instancedVbo);
    glBufferData(GL_ARRAY_BUFFER, stride * capacity, nullptr, GL_DYNAMIC_DRAW);

    // position
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(0));
    glVertexAttribDivisor(1, 1);

    // size
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    // color
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
    glVertexAttribDivisor(3, 1);

    // shape, mode, stroke, radius
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glVertexAttribDivisor(4, 1);
}


void pnt::SdfBuffer::bind()
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, instancedVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 12);
}