#ifndef __PHY_QUADTREE_H__
#define __PHY_QUADTREE_H__

#include <vector>

#include "vec2.h"
#include "geometry.h"

#include <iostream>
#include <concepts>

namespace phy {

    template<RectangularObjectConcept T>
    class Quadtree {

        int capacity;
        bool isDivided = false;
        std::vector<std::shared_ptr<Quadtree<T>>> children;
        std::vector<T*> objects;
        Rect2D boundary;
        float minArea;

        public:

            Quadtree() = default;
            
            void resize(const Rect2D& b, const int& c, const float& minArea = 1000)
            {
                capacity = c;
                boundary = b;
                this->minArea = minArea;
                children.clear();
                objects.clear();
            }

            void getRange(const Rect2D& range, std::vector<T*>& queried)
            {
                if(!rectToRectIntersect<Rect2D, Rect2D>(range, boundary)) return;

                for(auto& object: objects) {
                    if(rectToRectIntersect<T, Rect2D>(*object, range))
                        queried.push_back(object);
                }

                for(auto& child: children) child->getRange(range, queried);
            }

            bool insert(T* object) {
                if(!rectFitCompletely(object, boundary))
                    return false;

                if(objects.size() < capacity && children.empty()) {
                    objects.push_back(object);
                    return true;
                } else {
                    subdivide();
                }
                
                for(auto& child: children) 
                {
                    if(child->insert(object)) {
                        return true;
                    }
                }

                objects.push_back(object);

                return true;
            }

            const decltype(children)& getChildren() const {
                return children;
            }

            const Rect2D& getBoundary() const {
                return boundary;
            }

        private:
            void subdivide() {
                const auto bh = boundary.size * 0.5f;
                if(children.size() || bh.x * bh.y < minArea) return;

                children.emplace_back(new Quadtree<T>());
                children.emplace_back(new Quadtree<T>());
                children.emplace_back(new Quadtree<T>());
                children.emplace_back(new Quadtree<T>());

                const float hw = boundary.size.x * 0.5;
                const float hh = boundary.size.y * 0.5;
                children[0]->resize(phy::Rect2D{ {boundary.pos.x, boundary.pos.y}, { hw, hh } }, capacity, minArea);
                children[1]->resize(phy::Rect2D{ {boundary.pos.x + hw, boundary.pos.y}, { hw, hh } }, capacity, minArea);
                children[2]->resize(phy::Rect2D{ {boundary.pos.x, boundary.pos.y + hh}, { hw, hh } }, capacity, minArea);
                children[3]->resize(phy::Rect2D{ {boundary.pos.x + hw, boundary.pos.y + hh}, { hw, hh } }, capacity, minArea);

                redistributeObject();
            }

            void redistributeObject()
            {
                if(children.empty()) return;

                for(auto it = objects.begin(); it != objects.end(); )
                {
                    T* obj = *it;
                    bool moved = false;

                    // Try to move into exactly one child
                    for(auto& child : children)
                    {
                        if(child->rectFitCompletely(obj, child->boundary))
                        {
                            child->objects.push_back(obj);
                            it = objects.erase(it);  // erase safely
                            moved = true;
                            break; // IMPORTANT: stop after first match
                        }
                    }

                    if(!moved)
                        ++it; // stays in parent
                }
            }


            inline constexpr bool rectFitCompletely(const T* rect, const phy::Rect2D& boundary) {
                return rect->pos.x >= boundary.pos.x && (rect->pos.x + rect->size.x) <= boundary.pos.x + boundary.size.x
                    && rect->pos.y >= boundary.pos.y && (rect->pos.y + rect->size.y) <= boundary.pos.y + boundary.size.y;
            }

            template<RectangularObjectConcept A, RectangularObjectConcept B>
            inline constexpr bool rectToRectIntersect(const A& a, const B& b)
            {
                return (a.pos.x < b.pos.x + b.size.x && a.pos.x + a.size.x > b.pos.x && 
                    a.pos.y < b.pos.y + b.size.y && a.pos.y + a.size.y > b.pos.y);
                // return false;
            }
    };


}

#endif 