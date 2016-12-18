#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <array>
#include <functional>

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

/* Each leaf contains 1 by 1 area */

template <typename T>
class QuadtreeNode {
    public:
    QuadtreeNode(QuadtreeNode *parent, NodeArea area,
                 std::array<QuadtreeNode *, 4> children) :
        parent_(parent),
        area_(area),
        children_(children)
    {
        for (auto &child : children_) {
            if (!child)
                continue;
            child->parent_ = this;
        }
    }

    QuadtreeNode(NodeArea area) :
        QuadtreeNode(nullptr, area, {nullptr, nullptr, nullptr, nullptr})
    {
    }

    QuadtreeNode(NodeArea area, QuadtreeNode *parent) :
        parent_(parent),
        area_(area),
        children_{{nullptr, nullptr, nullptr, nullptr}}
    {
    }
 
    /* x and y are used for finding the place in the structure */
    QuadtreeNode *insert(T data, int x, int y)
    {
        if (!area_.is_inside(x, y)) {
            printf("not inside: %p, area: %d, %d, %d, %d\n", (void *)this, area_.x,
                   area_.y, area_.w, area_.h);
            assert(0);
        }

        if (area_.x == x && area_.y == y && area_.w == 1 && area_.h == 1) {
            data_ = data;
            return this;
        }

        auto q = area_.get_quadrant(x, y);
        assert(q != Quadrant::NONE);

        if (!children_[q])
            children_[q] = new QuadtreeNode(area_.get_quadrant_area(q), this);
        return children_[q]->insert(data, x, y);
    }

    const QuadtreeNode *search(int x, int y) const
    {
        auto c = children_[area_.get_quadrant(x, y)];

        if (x == area_.x && y == area_.y)
            return this;

        if (!c)
            return NULL;

        return c->search(x, y);
    }

    const QuadtreeNode *cache_search(int x, int y,
                                     std::function<T(int x, int y)> gen_data)
    {
        auto q = area_.get_quadrant(x, y);

        if (x == area_.x && y == area_.y)
            return this;

        if (!children_[q]) {
            return insert(gen_data(x, y), x, y);
        }

        return children_[q]->search(x, y);
    }

    const QuadtreeNode *get_child(Quadrant q) const
    {
        if (q == Quadrant::NONE)
            return NULL;
        return children_[q];
    }

    QuadtreeNode *get_child(Quadrant q) { return children_[q]; }
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
    QuadtreeNode<T> *parent_;
    NodeArea area_;
    std::array<QuadtreeNode<T> *, 4> children_;
    T data_;
};

template <typename T>
class Quadtree {
    public:
    Quadtree() :
        root_node_(NULL)
    {
    }

    Quadtree(int x, int y, int w, int h)
    {
        root_node_ = new QuadtreeNode<T>(NodeArea(x, y, w, h));
    }

    QuadtreeNode<T> *get_child(const Quadrant q)
    {
        if (!root_node_)
            return NULL;
        return root_node_->get_child(q);
    }
    void insert(T data, int x, int y)
    {
        if (!root_node_)
            root_node_ = new QuadtreeNode<T>(NodeArea(x, y, 1, 1));

        while (!root_node_->get_area().is_inside(x, y)) {
            const auto &area = root_node_->get_area();
            int nx = area.x;
            int ny = area.y;
            if (x < area.x || y < area.y) {
                nx -= area.w;
                ny -= area.h;
            }
            const NodeArea new_area(nx, ny, area.w * 2, area.h * 2);

            std::array<QuadtreeNode<T> *, 4> new_children {{NULL, NULL, NULL, NULL}};
            auto q = new_area.get_quadrant(area.x, area.y);
            assert(q != Quadrant::NONE);
            new_children[q] = root_node_;

            QuadtreeNode<T> *new_root_node = new QuadtreeNode<T>(nullptr,
                                                                 new_area,
                                                                 new_children);
            root_node_ = new_root_node;
        }

        root_node_->insert(data, x, y);
    }
    void print_status()
    {
        assert(root_node_);
        root_node_->print_status();
    }
    const QuadtreeNode<T> *get_root(void) const
    {
        return root_node_;
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
        if (!root_node_)
            return nullptr;
        return root_node_->cache_search(x, y, gen_data);
    }

    private:
    QuadtreeNode<T> *root_node_;
};

int main(int argc, char *argv[])
{
    {
        NodeArea a(0, 0, 1, 1);
        assert(a.is_inside(0, 0));
        assert(!a.is_inside(1, 1));
    }
    {
        NodeArea a(-1, -1, 2, 2);
        assert(a.is_inside(-1, -1));
        assert(a.is_inside(0, 0));
        assert(!a.is_inside(1, 1));
        assert(a.get_quadrant(-1, -1) == Quadrant::NW);
        assert(a.get_quadrant(0, 0) == Quadrant::SE);
        assert(a.get_quadrant(0, -1) == Quadrant::NE);
        assert(a.get_quadrant(-1, 0) == Quadrant::SW);
        assert(a.get_quadrant(2, 0) == Quadrant::NONE);
        assert(a.get_quadrant(0, 2) == Quadrant::NONE);
        assert(a.get_quadrant(2, 2) == Quadrant::NONE);
        assert(a.get_quadrant(-2, -2) == Quadrant::NONE);
    }
    {
        printf("Simple case\n");
        int se_data = 'se';
        QuadtreeNode<int> root(NodeArea(-1, -1, 2, 2));
        root.insert(se_data, 0, 0);
        assert(root.get_child(Quadrant::SE) != NULL);
        assert(root.get_child(Quadrant::SE)->get_data() == 'se');
        assert(root.get_child(Quadrant::NE) == NULL);
    }
    {
        printf("Multi level tree\n");
        int se_data = 'se';
        QuadtreeNode<int> root(NodeArea(-2, -2, 4, 4));
        root.insert(se_data, 0, 0);
        auto se = root.get_child(Quadrant::SE);
        assert(se != NULL);
        assert(se->get_child(Quadrant::NW) != NULL);
        assert(se->get_child(Quadrant::NW)->get_data() == 'se');
    }
    {
        printf("\nExpansion of root\n");
        int se_data = 'se';
        Quadtree<int> root(0, 0, 2, 2);

        root.insert(se_data, 2, 2);

        auto se = root.get_child(Quadrant::SE);

        /* The original root node */
        assert(root.get_child(Quadrant::NW) != NULL);
        /* The internal node on the way to (2, 2) */
        assert(se != NULL);

        /* Making sure parents are correct */
        assert(se->get_parent() == root.get_root());
        assert(root.get_child(Quadrant::NW)->get_parent() == root.get_root());

        auto senw = se->get_child(Quadrant::NW);

        /* The (2, 2) node */
        assert(senw != NULL);
        assert(senw->get_data() == 'se');

        /* Making sure parents are correct */
        assert(senw->get_parent() == root.get_child(Quadrant::SE));

    }
    {
        printf("\nMultilevel expansion of root\n");
        int se_data = 'se';
        Quadtree<int> root(-2, -2, 1, 1);

        root.insert(se_data, 2, 3);

        /* The original root node */
        assert(root.get_child(Quadrant::NW) != NULL);
        assert(root.get_child(Quadrant::NW)->get_child(Quadrant::NW) != NULL);
        /* The internal nodes on the way to (2, 3) */
        assert(root.get_child(Quadrant::SE) != NULL);
        auto senw = root.get_child(Quadrant::SE)->get_child(Quadrant::NW);
        assert(senw->get_child(Quadrant::SW) != NULL);
        assert(senw->get_child(Quadrant::SW)->get_data() == 'se');
    }
    {
        printf("\nExpansion of root into negative direction\n");
        int se_data = 'se';
        Quadtree<int> root(0, 0, 2, 2);

        root.insert(se_data, -2, -2);

        assert(root.get_child(Quadrant::NW) != NULL);
        auto nwnw = root.get_child(Quadrant::NW)->get_child(Quadrant::NW);
        assert(nwnw != NULL);
        assert(nwnw->get_data() == 'se');
    }
    {
        printf("\nMultilevel expansion of root into negative direction\n");
        int se_data = 'se';
        Quadtree<int> root(0, 0, 2, 2);

        root.insert(se_data, -3, -3);

        assert(root.get_child(Quadrant::NW) != NULL);
        auto nwse = root.get_child(Quadrant::NW)->get_child(Quadrant::SE);
        assert(nwse != NULL);
        assert(nwse->get_child(Quadrant::SE) != NULL);
        assert(nwse->get_child(Quadrant::SE)->get_data() == 'se');
    }
    {
        printf("\nMultilevel expansion of root with previous stuff\n");
        int first = 'fi';
        int second = 'se';
        Quadtree<int> root(-2, -2, 2, 2);

        root.insert(first, -2, -1);
        assert(root.get_child(Quadrant::SW)->get_data() == 'fi');

        root.insert(second, 2, 3);

        /* The original root node */
        assert(root.get_child(Quadrant::NW) != NULL);
        assert(root.get_child(Quadrant::NW)->get_child(Quadrant::NW) != NULL);

        assert(root.get_child(Quadrant::SE) != NULL);
        auto senw = root.get_child(Quadrant::SE)->get_child(Quadrant::NW);

        /* The internal nodes on the way to (2, 3) */
        assert(senw->get_child(Quadrant::SW) != NULL);
        assert(senw->get_child(Quadrant::SW)->get_data() == 'se');
    }
    {
        printf("\nSearch\n");
        int first = 'fi';
        int second = 'se';
        Quadtree<int> root(0, 0, 4, 4);

        root.insert(first, 1, 1);
        root.insert(second, 2, 1);
        assert(root.search(1, 1) != NULL);
        assert(root.search(2, 1) != NULL);
        assert(root.search(1, 2) == NULL);
        assert(root.search(1, 1)->get_data() == first);
        assert(root.search(2, 1)->get_data() == second);
    }
    {
        printf("\nSearch after expansion\n");
        int first = 'fi';
        int second = 'se';
        Quadtree<int> root(-2, -2, 2, 2);

        root.insert(first, -2, -1);
        assert(root.get_child(Quadrant::SW)->get_data() == 'fi');

        root.insert(second, 2, 3);

        /* The original root node */
        assert(root.get_child(Quadrant::NW) != NULL);
        assert(root.get_child(Quadrant::NW)->get_child(Quadrant::NW) != NULL);

        assert(root.get_child(Quadrant::SE) != NULL);
        auto senw = root.get_child(Quadrant::SE)->get_child(Quadrant::NW);

        /* The internal nodes on the way to (2, 3) */
        assert(senw->get_child(Quadrant::SW) != NULL);
        assert(senw->get_child(Quadrant::SW)->get_data() == 'se');

        assert(root.search(-2, -1) != NULL);
        assert(root.search(2, 3) != NULL);
        assert(root.search(1, 2) == NULL);
        assert(root.search(-2, -1)->get_data() == first);
        assert(root.search(2, 3)->get_data() == second);
    }
    {
        printf("\nCache search\n");
        int first = 'fi';
        int second = 'se';
        Quadtree<int> root(0, 0, 4, 4);

        assert(root.search(1, 1) == NULL);
        assert(root.search(2, 1) == NULL);
        root.cache_search(1, 1, [=](int x, int y) { return first; });
        assert(root.search(1, 1) != NULL);
        root.cache_search(2, 1, [=](int x, int y) { return second; });
        assert(root.search(2, 1) != NULL);
        assert(root.search(1, 2) == NULL);
        assert(root.search(1, 1)->get_data() == first);
        assert(root.search(2, 1)->get_data() == second);
    }
    return EXIT_SUCCESS;
}
