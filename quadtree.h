#pragma once
#ifndef QUADTREE_H
#define QUADTREE_H

#include <array>
#include <functional>
#include <memory>

/*     |
 *  NW | NE x
 * ----+---->
 *  SW | SE
 *     |
 *     v y
 */
enum Quadrant : ssize_t {
    NONE = -1, NW = 0, SW = 1, NE = 2, SE = 3
};

struct NodeArea
{
    NodeArea(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) { }
    inline bool is_inside(int qx, int qy) const
    {
        return qx >= x && qx < x + w && qy >= y && qy < y + h;
    }
    inline bool is_inside(const NodeArea &node) const
    {
        return is_inside(node.x, node.y) &&
               is_inside(node.x + node.w, node.y + node.h);
    }
    inline Quadrant get_quadrant(int qx, int qy) const
    {
        const int cx = x + w / 2;
        const int cy = y + h / 2;

        if (!is_inside(qx, qy))
            return Quadrant::NONE;

        enum Direction {
            N = 0, S = 1, W = 0, E = 2
        };

        return Quadrant((qx >= cx ? E : W) | (qy >= cy ? S : N));
    }
    inline NodeArea get_quadrant_area(Quadrant q) const
    {
        const int hw = w / 2;
        const int hh = h / 2;
        int ry = y, rx = x;

        if (q != Quadrant::NW && q != Quadrant::SW)
            rx += hw;
        if (q != Quadrant::NW && q != Quadrant::NE)
            ry += hh;

        return NodeArea(rx, ry, hw, hh);
    }
    int x, y;
    int w, h;
};

template <typename T>
class QuadtreeNode {
    public:
    QuadtreeNode(NodeArea area,
                 std::array<std::unique_ptr<QuadtreeNode>, 4> &children) :
        parent_(nullptr),
        area_(area)
    {
        for (size_t i = 0; i < children_.size(); ++i) {
            if (!children[i])
                continue;
            children_[i] = std::move(children[i]);
            children_[i]->parent_ = this;
        }
    }

    QuadtreeNode(NodeArea area) : QuadtreeNode(area, nullptr) { }

    QuadtreeNode(NodeArea area, QuadtreeNode *parent) :
        parent_(parent),
        area_(area),
        children_{{nullptr, nullptr, nullptr, nullptr}}
    {
    }
 
    /* x and y are used for finding the place in the structure */
    QuadtreeNode *insert(int x, int y, T data)
    {
        if (!area_.is_inside(x, y))
            throw std::logic_error("Requested point not inside the search area");

        if (area_.x == x && area_.y == y && area_.w == 1 && area_.h == 1) {
            data_ = data;
            return this;
        }

        auto q = area_.get_quadrant(x, y);
        if (q == Quadrant::NONE)
            throw std::logic_error("Could not find the correct quadrant");

        if (!children_[q]) {
            auto area = area_.get_quadrant_area(q);
            children_[q] = std::make_unique<QuadtreeNode>(area, this);
        }

        return children_[q]->insert(x, y, std::move(data));
    }

    const QuadtreeNode *search(int x, int y) const
    {
        auto c = children_[area_.get_quadrant(x, y)].get();

        if (x == area_.x && y == area_.y)
            return this;

        if (!c)
            return nullptr;

        return c->search(x, y);
    }

    const QuadtreeNode *cache_search(int x, int y,
                                     std::function<T(int x, int y)> gen_data)
    {
        auto q = area_.get_quadrant(x, y);

        if (x == area_.x && y == area_.y)
            return this;

        if (!children_[q])
            return insert(x, y, gen_data(x, y));

        return children_[q]->search(x, y);
    }

    const QuadtreeNode *get_child(Quadrant q) const
    {
        if (q == Quadrant::NONE)
            return NULL;
        return children_[q];
    }

    QuadtreeNode *get_child(Quadrant q) { return children_[q].get(); }
    const T get_data() const { return data_; }
    const NodeArea &get_area() const { return area_; }
    const QuadtreeNode *get_parent() const { return parent_; }
    void print_status() const
    {
        printf("this: %p\n", this);
        printf("area: x: %d, y: %d, w: %d, h: %d\n", area_.x, area_.y, area_.h,
               area_.w);
        printf("NW: %p\n", children_[Quadrant::NW]);
        printf("NE: %p\n", children_[Quadrant::NE]);
        printf("SW: %p\n", children_[Quadrant::SW]);
        printf("SE: %p\n", children_[Quadrant::SE]);
    }

    private:
    QuadtreeNode *parent_;
    NodeArea area_;
    std::array<std::unique_ptr<QuadtreeNode>, 4> children_;
    T data_;
};

template <typename T>
class Quadtree {
    public:
    Quadtree() : root_node_(nullptr) { }

    Quadtree(int x, int y, int w, int h)
    {
        root_node_ = std::make_unique<QuadtreeNode<T>>(NodeArea(x, y, w, h));
    }

    QuadtreeNode<T> *get_child(const Quadrant q)
    {
        if (!root_node_)
            return NULL;
        return root_node_->get_child(q);
    }

    QuadtreeNode<T> *insert(int x, int y, T data)
    {
        if (!root_node_)
            root_node_ = std::make_unique<QuadtreeNode<T>>(NodeArea(x, y, 1, 1));

        expand_root_(x, y);

        return root_node_->insert(x, y, std::move(data));
    }

    void print_status()
    {
        if (!root_node_)
            printf("root node: NULL\n");
        root_node_->print_status();
    }

    const QuadtreeNode<T> *get_root(void) const
    {
        return root_node_.get();
    }

    const QuadtreeNode<T> *search(int x, int y) const
    {
        if (!root_node_)
            return nullptr;
        return root_node_->search(x, y);
    }

    /*
     * The quadtree is used as a cache. If there's a miss, the entry is
     * generated by calling gen_data. Otherwise the previously generated data
     * is returned.
     */
    const QuadtreeNode<T> *cache_search(int x, int y,
                                        std::function<T(int x, int y)> gen_data)
    {
        if (!root_node_) {
            root_node_ = std::make_unique<QuadtreeNode<T>>(NodeArea(x, y, 1, 1));
            expand_root_(x, y);
            return insert(x, y, gen_data(x, y));
        }

        expand_root_(x, y);
        return root_node_->cache_search(x, y, gen_data);
    }

    private:
    void expand_root_(int x, int y)
    {
        while (!root_node_->get_area().is_inside(x, y)) {
            const auto &area = root_node_->get_area();
            int nx = area.x;
            int ny = area.y;
            int nw = (area.w ? area.w : 1) * 2;
            int nh = (area.h ? area.h : 1) * 2;
            if (x < area.x || y < area.y) {
                nx -= area.w;
                ny -= area.h;
            }
            const NodeArea new_area(nx, ny, nw, nh);

            std::array<std::unique_ptr<QuadtreeNode<T>>, 4> new_children{};
            auto q = new_area.get_quadrant(area.x, area.y);
            if (q == Quadrant::NONE)
                throw std::logic_error("Could not find the correct quadrant");
            new_children[q] = std::move(root_node_);

            root_node_ = std::make_unique<QuadtreeNode<T>>(new_area, new_children);
        }
    }

    std::unique_ptr<QuadtreeNode<T>> root_node_;
};

#endif
