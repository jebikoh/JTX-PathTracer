#pragma once

#include "rt.hpp"
#include "util/interval.hpp"
#include "material.hpp"
#include "scene.hpp"
#include "primitives.hpp"

class HittableList;
class BVHNode;

class PrimitiveList {
public:
    explicit PrimitiveList(const Scene &scene) : scene_(scene) {
        for (int i = 0; i < scene.spheres.size(); ++i) {
            primitives_.push_back({Primitive::SPHERE, i});
        }
    }

    bool hit(const Ray &r, const Interval t, HitRecord &record) const {
        HitRecord tmpRecord{};
        bool hitAnything   = false;
        float closestSoFar = t.max;

        for (const auto &prim : primitives_) {
            switch(prim.type) {
                case Primitive::SPHERE: {
                    if (const auto &sphere = scene_.spheres[prim.index]; sphere.hit(r, Interval(t.min, closestSoFar), tmpRecord)) {
                        hitAnything  = true;
                        closestSoFar = tmpRecord.t;
                        record       = tmpRecord;
                    }
                    break;
                }
                case Primitive::RECTANGLE:
                    break;
                default:
                    break;
            }
        }

        return hitAnything;
    }
private:
    const Scene &scene_;
    std::vector<Primitive> primitives_;
};
//
// class BVHNode {
// public:
//     BVHNode(HittableList &list) : BVHNode(list.objects_, 0, list.objects_.size()) {}
//
//     BVHNode(std::vector<Hittable> &objects, const size_t start, const size_t end) {
//         bbox_ = AABB::EMPTY;
//         for (auto i = start; i < end; ++i) {
//             bbox_.expand(objects[i].bounds());
//         }
//         const int axis = bbox_.longestAxis();
//
//         const auto comparator = [axis](const Hittable &a, const Hittable &b) {
//             return a.bounds().pmin[axis] < b.bounds().pmin[axis];
//         };
//
//         const size_t numObjects = end - start;
//         if (numObjects == 1) {
//             left_ = right_ = objects[start];
//         } else if (numObjects == 2) {
//             left_  = objects[start];
//             right_ = objects[start + 1];
//         } else {
//             std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);
//             const auto mid = start + numObjects / 2;
//
//             left_  = Hittable(new BVHNode(objects, start, mid));
//             right_ = Hittable(new BVHNode(objects, mid, end));
//         }
//     }
//
//     bool hit(const Ray &r, const Interval t, HitRecord &record) const {
//         if (!bbox_.hit(r.origin, r.dir, t)) {
//             return false;
//         }
//
//         const bool hitLeft = left_.hit(r, t, record);
//         const bool hitRight = right_.hit(r, Interval(t.min, hitLeft ? record.t : t.max), record);
//         return hitLeft || hitRight;
//     }
//
//     AABB bounds() const {
//         return bbox_;
//     }
//
//     friend void destroyBVHTree(BVHNode *root, bool freeRoot);
// private:
//     Hittable left_;
//     Hittable right_;
//     AABB bbox_;
// };
//
// // This should only be called on the BVH root
// // If the root BVHNode is also called on heap, the user is responsible for freeing the memory
// inline void destroyBVHTree(BVHNode *root, const bool freeRoot = false) {
//     std::vector<Hittable> stack;
//
//     // Initialize our stack
//     if (freeRoot) {
//         stack.push_back(Hittable(root));
//     } else {
//         if (root->left_.getTag() == BVH_TAG_IDX) {
//             stack.push_back(root->left_);
//         }
//         if (root->right_.getTag() == BVH_TAG_IDX) {
//             stack.push_back(root->right_);
//         }
//     }
//
//     // Loop through the BVH tree and process any objects that are BVHNodes
//     while(!stack.empty()) {
//         auto curr = stack.back();
//         stack.pop_back();
//
//         if (curr.getTag() == BVH_TAG_IDX) {
//             const auto node = curr.cast<BVHNode>();
//             if (node->left_.getTag() == BVH_TAG_IDX) {
//                 stack.push_back(node->left_);
//             }
//             if (node->right_.getTag() == BVH_TAG_IDX) {
//                 stack.push_back(node->right_);
//             }
//             delete node;
//         }
//     }
// }
