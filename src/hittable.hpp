#pragma once

#include "aabb.hpp"
#include "hit.hpp"
#include "interval.hpp"
#include "material.hpp"
#include "rt.hpp"
#include <jtxlib/util/taggedptr.hpp>


class Sphere;
class HittableList;
class BVHNode;

class Hittable : public jtx::TaggedPtr<Sphere, HittableList, BVHNode> {
public:
    using TaggedPtr::TaggedPtr;

    bool hit(const Ray &r, Interval t, HitRecord &record) const;

    AABB bounds() const;
};

constexpr unsigned int BVH_TAG_IDX = Hittable::tagIndex<BVHNode>();

class Sphere {
public:
    Sphere(const Vec3 &center, const Float radius, const Material &material)
        : center_(center, Vec3(0, 0, 0)),
          radius_(radius),
          material_(material) {
        const auto r = Vec3(radius, radius, radius);
        bbox_ = AABB(center - r, center + r);
    }

    // Moving spheres
    Sphere(const Vec3 &start, const Vec3 &end, Float radius, const Material &material)
        : center_(start, end - start),
          radius_(radius),
          material_(material) {
        const auto r = Vec3(radius, radius, radius);
        bbox_ = AABB(center_.at(0) - r, center_.at(0) + r);
        bbox_.expand(AABB(center_.at(1) - r, center_.at(1) + r));
    }

    bool hit(const Ray &r, const Interval t, HitRecord &record) const {
        const auto currentCenter = center_.at(r.time);
        const Vec3 oc            = currentCenter - r.origin;
        const Float a            = r.dir.lenSqr();
        const Float h            = jtx::dot(r.dir, oc);
        const Float c            = oc.lenSqr() - radius_ * radius_;

        const Float discriminant = h * h - a * c;
        if (discriminant < 0) {
            return false;
        }

        const auto sqrtd = jtx::sqrt(discriminant);
        auto root        = (h - sqrtd) / a;
        if (!t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!t.surrounds(root)) {
                return false;
            }
        }

        record.t     = root;
        record.point = r.at(root);
        const auto n = (record.point - currentCenter) / radius_;
        record.setFaceNormal(r, n);
        record.material = &material_;

        return true;
    }

    AABB bounds() const {
        return bbox_;
    }

private:
    Ray center_;
    Float radius_;
    Material material_;
    AABB bbox_;
};

class HittableList {
public:
    HittableList() = default;
    explicit HittableList(const Hittable &object) { add(object); }

    void add(const Hittable &object) {
        objects_.push_back(object);
        bbox_.expand(object.bounds());
    }

    void clear() { objects_.clear(); }

    bool hit(const Ray &r, const Interval t, HitRecord &record) const {
        HitRecord tmpRecord{};
        bool hitAnything  = false;
        auto closestSoFar = t.max;

        for (const auto &object: objects_) {
            if (object.hit(r, Interval(t.min, closestSoFar), tmpRecord)) {
                hitAnything  = true;
                closestSoFar = tmpRecord.t;
                record       = tmpRecord;
            }
        }

        return hitAnything;
    }

    AABB bounds() const {
        return bbox_;
    }

    friend class BVHNode;
private:
    std::vector<Hittable> objects_;
    AABB bbox_ = AABB::EMPTY;
};

class BVHNode {
public:
    BVHNode(HittableList &list) : BVHNode(list.objects_, 0, list.objects_.size()) {}

    BVHNode(std::vector<Hittable> &objects, const size_t start, const size_t end) {
        bbox_ = AABB::EMPTY;
        for (auto i = start; i < end; ++i) {
            bbox_.expand(objects[i].bounds());
        }
        const int axis = bbox_.longestAxis();

        const auto comparator = [axis](const Hittable &a, const Hittable &b) {
            return a.bounds().pmin[axis] < b.bounds().pmin[axis];
        };

        const size_t numObjects = end - start;
        if (numObjects == 1) {
            left_ = right_ = objects[start];
        } else if (numObjects == 2) {
            left_  = objects[start];
            right_ = objects[start + 1];
        } else {
            std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);
            const auto mid = start + numObjects / 2;

            left_  = Hittable(new BVHNode(objects, start, mid));
            right_ = Hittable(new BVHNode(objects, mid, end));
        }
    }

    bool hit(const Ray &r, const Interval t, HitRecord &record) const {
        if (!bbox_.hit(r.origin, r.dir, t)) {
            return false;
        }

        const bool hitLeft = left_.hit(r, t, record);
        const bool hitRight = right_.hit(r, Interval(t.min, hitLeft ? record.t : t.max), record);
        return hitLeft || hitRight;
    }

    AABB bounds() const {
        return bbox_;
    }

    friend void destroyBVHTree(BVHNode *root, bool freeRoot);
private:
    Hittable left_;
    Hittable right_;
    AABB bbox_;
};

// This should only be called on the BVH root
// If the root BVHNode is also called on heap, the user is responsible for freeing the memory
inline void destroyBVHTree(BVHNode *root, const bool freeRoot = false) {
    std::vector<Hittable> stack;

    // Initialize our stack
    if (freeRoot) {
        stack.push_back(Hittable(root));
    } else {
        if (root->left_.getTag() == BVH_TAG_IDX) {
            stack.push_back(root->left_);
        }
        if (root->right_.getTag() == BVH_TAG_IDX) {
            stack.push_back(root->right_);
        }
    }

    // Loop through the BVH tree and process any objects that are BVHNodes
    while(!stack.empty()) {
        auto curr = stack.back();
        stack.pop_back();

        if (curr.getTag() == BVH_TAG_IDX) {
            const auto node = curr.cast<BVHNode>();
            if (node->left_.getTag() == BVH_TAG_IDX) {
                stack.push_back(node->left_);
            }
            if (node->right_.getTag() == BVH_TAG_IDX) {
                stack.push_back(node->right_);
            }
            delete node;
        }
    }
}

inline bool Hittable::hit(const Ray &r, Interval t, HitRecord &record) const {
    auto fn = [&](auto ptr) { return ptr->hit(r, t, record); };
    return dispatch(fn);
}

inline AABB Hittable::bounds() const {
    auto fn = [&](auto ptr) { return ptr->bounds(); };
    return dispatch(fn);
}

