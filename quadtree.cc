#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
    NodeArea() : NodeArea(0, 0, 0, 0) { }
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
    QuadtreeNode(int x, int y, int w, int h) :
        QuadtreeNode(x, y, w, h, NULL, NULL, NULL, NULL)
    {
    }

    QuadtreeNode(int x, int y, int w, int h, QuadtreeNode *c_nw, QuadtreeNode *c_ne,
             QuadtreeNode *c_sw, QuadtreeNode *c_se) :
        parent_(NULL),
        area_(x, y, w, h),
        children_{c_nw, c_ne, c_sw, c_se}
    {
    }

    QuadtreeNode(NodeArea area, QuadtreeNode **children) :
        parent_(NULL),
        area_(area)
    {
        children_[0] = children[0];
        children_[1] = children[1];
        children_[2] = children[2];
        children_[3] = children[3];
    }

    QuadtreeNode(NodeArea area, QuadtreeNode *parent, QuadtreeNode *c_nw, QuadtreeNode *c_ne,
             QuadtreeNode *c_sw, QuadtreeNode *c_se) :
        parent_(parent),
        area_(area),
        children_{c_nw, c_ne, c_sw, c_se}
    {
        /*
        printf("%s: ax: %d, ay: %d, aw: %d, ah: %d\n", __func__, area.x,
               area.y, area.w, area.h);
        printf("%s: ax: %d, ay: %d, aw: %d, ah: %d\n", __func__, area_.x,
               area_.y, area_.w, area_.h);
        */
    }

    QuadtreeNode(NodeArea area, QuadtreeNode *parent) :
        parent_(parent),
        area_(area),
        children_{NULL, NULL, NULL, NULL}
    {
        /*
        printf("%s: ax: %d, ay: %d, aw: %d, ah: %d\n", __func__, area.x,
               area.y, area.w, area.h);
        printf("%s: ax: %d, ay: %d, aw: %d, ah: %d\n", __func__, area_.x,
               area_.y, area_.w, area_.h);
        */
    }
 
    QuadtreeNode(T data) :
        parent_(NULL),
        data_(data)
    {
    }

    /* x and y are used for finding the place in the structure */
    void insert(T data, int x, int y)
    {
        /*
        printf("%s: x: %d, y: %d, ax: %d, ay: %d, aw: %d, ah: %d\n",
                __func__, x, y, area_.x, area_.y, area_.w, area_.h);
        printf("parent_: %p\n", parent_);
        */
        if (!area_.is_inside(x, y)) {
            printf("not inside: %p, area: %d, %d, %d, %d\n", this, area_.x,
                   area_.y, area_.w, area_.h);
            assert(0);
        }

        if (area_.x == x && area_.y == y && area_.w == 1 && area_.h == 1) {
            data_ = data;
            return;
        }

        auto q = area_.get_quadrant(x, y);
        assert(q != Quadrant::NONE);

        if (!children_[q])
            children_[q] = new QuadtreeNode(area_.get_quadrant_area(q), this);
        children_[q]->insert(data, x, y);
    }

    const QuadtreeNode *get_child(Quadrant q) const
    {
        if (q == Quadrant::NONE)
            return NULL;
        return children_[q];
    }

    QuadtreeNode *get_child(Quadrant q) { return children_[q]; }
    T get_data() { return data_; }
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
    const NodeArea &get_area() const
    {
        return area_;
    }

    QuadtreeNode<T> *parent_;
    private:
    /* Inner node */
    QuadtreeNode(QuadtreeNode *parent, T *c_nw, T *c_ne, T *c_sw, T *c_se) :
        parent_(parent),
        children_{c_nw, c_ne, c_sw, c_se}
    {
    }
    NodeArea area_;
    QuadtreeNode<T> *children_[4];
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
        root_node_ = new QuadtreeNode<T>(x, y, w, h);
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
            root_node_ = new QuadtreeNode<T>(x, y, 1, 1);

        /*
        printf("inserting into %d, %d\n", x, y);
        */

        while (!root_node_->get_area().is_inside(x, y)) {
            const auto &area = root_node_->get_area();
            int nx = area.x;
            int ny = area.y;
            if (x < area.x || y < area.y) {
                nx -= area.w;
                ny -= area.h;
            }
            const NodeArea new_area(nx, ny, area.w * 2, area.h * 2);

            /*
            printf("expanding the root from:\t%d,\t%d,\t%d,\t%d\n", area.x, area.y, area.w, area.h);
            printf("to:\t\t\t\t%d,\t%d,\t%d,\t%d\n", new_area.x, new_area.y, new_area.w, new_area.h);
            */

            QuadtreeNode<T> *new_children[4] = {NULL, NULL, NULL, NULL}; 
            auto q = new_area.get_quadrant(area.x, area.y);
            assert(q != Quadrant::NONE);
            new_children[q] = root_node_;

            QuadtreeNode<T> *new_root_node = new QuadtreeNode<T>(new_area,
                                                                 new_children);
            root_node_->parent_ = new_root_node;
            root_node_ = new_root_node;
        }

        root_node_->insert(data, x, y);
    }
    void print_status()
    {
        assert(root_node_);
        root_node_->print_status();
    }
    private:
    QuadtreeNode<T> *root_node_;
};

int main(int argc, char *argv[])
{
    {
        NodeArea a (0, 0, 1, 1);
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
        QuadtreeNode<int> root(-1, -1, 2, 2);
        root.insert(se_data, 0, 0);
        assert(root.get_child(Quadrant::SE) != NULL);
        assert(root.get_child(Quadrant::SE)->get_data() == 'se');
        assert(root.get_child(Quadrant::NE) == NULL);
    }
    {
        printf("Multi level tree\n");
        int se_data = 'se';
        QuadtreeNode<int> root(-2, -2, 4, 4);
        root.insert(se_data, 0, 0);
        assert(root.get_child(Quadrant::SE) != NULL);
        assert(root.get_child(Quadrant::SE)->get_child(Quadrant::NW) != NULL);
        assert(root.get_child(Quadrant::SE)->get_child(Quadrant::NW)->get_data() == 'se');
    }
    {
        printf("\nExpansion of root\n");
        int se_data = 'se';
        Quadtree<int> root(0, 0, 2, 2);

        root.insert(se_data, 2, 2);

        /* The original root node */
        assert(root.get_child(Quadrant::NW) != NULL);
        /* The internal node on the way to (2, 2) */
        assert(root.get_child(Quadrant::SE) != NULL);

        /* The (2, 2) node */
        assert(root.get_child(Quadrant::SE)->get_child(Quadrant::NW) != NULL);
        assert(root.get_child(Quadrant::SE)->get_child(Quadrant::NW)->get_data() == 'se');
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
        assert(root.get_child(Quadrant::SE)->get_child(Quadrant::NW)->get_child(Quadrant::SW) != NULL);
        assert(root.get_child(Quadrant::SE)->get_child(Quadrant::NW)->get_child(Quadrant::SW)->get_data() == 'se');
    }
    {
        printf("\nExpansion of root into negative direction\n");
        int se_data = 'se';
        Quadtree<int> root(0, 0, 2, 2);

        root.insert(se_data, -2, -2);

        assert(root.get_child(Quadrant::NW) != NULL);
        assert(root.get_child(Quadrant::NW)->get_child(Quadrant::NW) != NULL);
        assert(root.get_child(Quadrant::NW)->get_child(Quadrant::NW)->get_data() == 'se');
    }
    {
        printf("\nMultilevel expansion of root into negative direction\n");
        int se_data = 'se';
        Quadtree<int> root(0, 0, 2, 2);

        root.insert(se_data, -3, -3);

        assert(root.get_child(Quadrant::NW) != NULL);
        assert(root.get_child(Quadrant::NW)->get_child(Quadrant::SE) != NULL);
        assert(root.get_child(Quadrant::NW)->get_child(Quadrant::SE)->get_child(Quadrant::SE) != NULL);
        assert(root.get_child(Quadrant::NW)->get_child(Quadrant::SE)->get_child(Quadrant::SE)->get_data() == 'se');
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
        /* The internal nodes on the way to (2, 3) */
        assert(root.get_child(Quadrant::SE) != NULL);
        assert(root.get_child(Quadrant::SE)->get_child(Quadrant::NW)->get_child(Quadrant::SW) != NULL);
        assert(root.get_child(Quadrant::SE)->get_child(Quadrant::NW)->get_child(Quadrant::SW)->get_data() == 'se');
    }
    return EXIT_SUCCESS;
}
