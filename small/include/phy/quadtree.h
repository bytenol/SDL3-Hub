#ifndef __PHY_QUADTREE_H__
#define __PHY_QUADTREE_H__

#include <vector>



namespace phy {

    template<typename T>
    class Quadtree {

        int capacity = 4;
        
        SDL_FRect boundary;

        public:
            std::vector<T> objects;
            bool isDivided = false;

            std::vector<Quadtree> children;

            Quadtree() = default;

            Quadtree(const SDL_FRect& r, int maxObj = 4) {
                boundary = r;
                capacity = maxObj;
            }

            void insert(const T& point) {
                if(!pointInRect(point, boundary)) return;

                if(objects.size() < capacity) {
                    objects.emplace_back(point);
                } else {
                    if(!isDivided) {
                        isDivided = true;
                        const float wHalf = boundary.w / 2;
                        const float hHalf = boundary.h / 2;
                        children.push_back(Quadtree({ boundary.x, boundary.y, wHalf, hHalf }, capacity));
                        children.push_back(Quadtree({ boundary.x + wHalf, boundary.y, wHalf, hHalf }, capacity));
                        children.push_back(Quadtree({ boundary.x, boundary.y + hHalf, wHalf, hHalf }, capacity));
                        children.push_back(Quadtree({ boundary.x + wHalf, boundary.y + hHalf, wHalf, hHalf }, capacity));

                        for(auto it = objects.begin(); it != objects.end(); it++) {
                            for(int i = 0; i < children.size(); i++) {
                                auto& child = children[i];
                                child.insert(*it);
                            }
                        }

                        objects.clear();

                    }   // if !isDivided ends
                }

                if(isDivided) {
                    for(auto& child: children) {
                        child.insert(point);
                    }
                }

            }

            std::vector<T> findObject(const decltype(boundary)& range) {
                if(!rectToRectIntersect(boundary, range)) return {};

                std::vector<T> res;

                if(!isDivided) {
                    for(auto& pt: objects) {
                        if(pointInRect(pt, range)) res.emplace_back(pt);
                    }
                    return res;
                }

                else {
                    for(auto& child: children) {
                        auto pt = child.findObject(range);
                        res.insert(res.begin(), pt.begin(), pt.end());
                    }
                }

                return res;
            }

            size_t size() const {
                return objects.size();
            }

            // void render(SDL_Renderer* renderer) {
            //     SDL_RenderRect(renderer, &boundary);
            //     for(auto& child: children) 
            //         child.render(renderer);

            //     for(auto& point: objects) {
            //         drawFilledCircle(renderer, point.x, point.y, 1);
            //     }
            // }
    };


}

#endif 