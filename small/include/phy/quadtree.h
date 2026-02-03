#ifndef __PHY_QUADTREE_H__
#define __PHY_QUADTREE_H__

#include <vector>

#include "vec2.h"
#include "geometry.h"

#include <iostream>


namespace phy {

    template<typename T>
    class Quadtree {

        int capacity, maxDepth, minNodeSize;
        bool isDivided = false;
        std::vector<std::shared_ptr<Quadtree>> children;
        std::vector<T*> objects;
        Rect2D boundary;

        public:

            Quadtree() = default;
            
            void resize(const Rect2D& b, const int& c, const int& depth = 4, const int& minSize = 30)
            {
                capacity = c;
                boundary = b;
                maxDepth = depth;
                minNodeSize = minSize;
                isDivided = false;
                children.clear();
                objects.clear();
            }

            bool insert(T* object) {
                if(!rectFitCompletely(object, boundary))
                    return false;

                if(objects.size() < capacity && !children.size()) {
                    objects.emplace_back(object);
                    return true;
                } else {
                    if(boundary.size.x >= minNodeSize && boundary.size.y >= minNodeSize && !children.size()) 
                        subdivide();
                }
                
                // bool isFit = false;
                // if(children.size()) {
                //     for(auto& child: children) {
                //         if(child->insert(object)) {
                //             isFit = true;
                //             break;
                //         }
                //     }

                //     if(!isFit) objects.emplace_back(object);

                // } else {
                //     objects.emplace_back(object);
                //     return true;
                // }


                
                return true;
            }

            const decltype(objects)& getObjects() const {
                return objects;
            }

            const decltype(children)& getChildren() const {
                return children;
            }

            const Rect2D& getBoundary() const {
                return boundary;
            }

        private:
            void subdivide() {
                children.clear();
                children.emplace_back(new Quadtree());
                children.emplace_back(new Quadtree());
                children.emplace_back(new Quadtree());
                children.emplace_back(new Quadtree());

                const float hw = boundary.size.x * 0.5;
                const float hh = boundary.size.y * 0.5;
                children[0]->resize(phy::Rect2D{ {boundary.pos.x, boundary.pos.y}, { hw, hh } }, capacity, maxDepth, minNodeSize);
                children[1]->resize(phy::Rect2D{ {boundary.pos.x + hw, boundary.pos.y}, { hw, hh } }, capacity, maxDepth, minNodeSize);
                children[2]->resize(phy::Rect2D{ {boundary.pos.x, boundary.pos.y + hh}, { hw, hh } }, capacity, maxDepth, minNodeSize);
                children[3]->resize(phy::Rect2D{ {boundary.pos.x + hw, boundary.pos.y + hh}, { hw, hh } }, capacity, maxDepth, minNodeSize);

                redistributeObject();

                std::cout << objects.size() << std::endl;

                isDivided = true;
            }

            void redistributeObject() {
                for(auto it = objects.begin(); it != objects.end(); ) {
                    auto& object = *it;
                    bool moved = false;

                    for(auto& child: children) {
                        if(rectFitCompletely(object, child->boundary)) {
                            child->objects.emplace_back(object);
                            it = objects.erase(it);
                            break;
                        }
                    }

                    if(!moved) ++it;
                }
            }

            inline constexpr bool rectFitCompletely(T* rect, const phy::Rect2D& boundary) {
                return rect->pos.x >= boundary.pos.x && (rect->pos.x + rect->size.x) <= boundary.pos.x + boundary.size.x
                    && rect->pos.y >= boundary.pos.y && (rect->pos.y + rect->size.y) <= boundary.pos.y + boundary.size.y;
            }
    };


}

#endif 