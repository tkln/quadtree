#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "quadtree.h"

int main(int argc, char *argv[])
{
    using namespace quadtree;
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
        root.insert(0, 0, se_data);
        assert(root.get_child(Quadrant::SE) != NULL);
        assert(root.get_child(Quadrant::SE)->get_data() == 'se');
        assert(root.get_child(Quadrant::NE) == NULL);
    }
    {
        printf("Multi level tree\n");
        int se_data = 'se';
        QuadtreeNode<int> root(NodeArea(-2, -2, 4, 4));
        root.insert(0, 0, se_data);
        auto se = root.get_child(Quadrant::SE);
        assert(se != NULL);
        assert(se->get_child(Quadrant::NW) != NULL);
        assert(se->get_child(Quadrant::NW)->get_data() == 'se');
    }
    {
        printf("\nExpansion of root\n");
        int se_data = 'se';
        auto root_node = std::make_unique<QuadtreeNode<int>>(NodeArea(0, 0, 2, 2));
        Quadtree<int> root(std::move(root_node));

        root.insert(2, 2, se_data);

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
        auto area = NodeArea(-2, -2, 1, 1);
        auto root_node = std::make_unique<QuadtreeNode<int>>(area);
        Quadtree<int> root(std::move(root_node));

        root.insert(2, 3, se_data);

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
        auto area = NodeArea(0, 0, 2, 2);
        auto root_node = std::make_unique<QuadtreeNode<int>>(area);
        Quadtree<int> root(std::move(root_node));

        root.insert(-2, -2, se_data);

        assert(root.get_child(Quadrant::NW) != NULL);
        auto nwnw = root.get_child(Quadrant::NW)->get_child(Quadrant::NW);
        assert(nwnw != NULL);
        assert(nwnw->get_data() == 'se');
    }
    {
        printf("\nMultilevel expansion of root into negative direction\n");
        int se_data = 'se';
        auto area = NodeArea(0, 0, 2, 2);
        auto root_node = std::make_unique<QuadtreeNode<int>>(area);
        Quadtree<int> root(std::move(root_node));

        root.insert(-3, -3, se_data);

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
        auto area = NodeArea(-2, -2, 2, 2);
        auto root_node = std::make_unique<QuadtreeNode<int>>(area);
        Quadtree<int> root(std::move(root_node));

        root.insert(-2, -1, first);
        assert(root.get_child(Quadrant::SW)->get_data() == 'fi');

        root.insert(2, 3, second);

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
        Quadtree<int> root/*(0, 0, 4, 4)*/;

        root.insert(1, 1, first);
        root.insert(2, 1, second);
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
        auto area = NodeArea(-2, -2, 2, 2);
        auto root_node = std::make_unique<QuadtreeNode<int>>(area);
        Quadtree<int> root(std::move(root_node));

        root.insert(-2, -1, first);
        assert(root.get_child(Quadrant::SW)->get_data() == 'fi');

        root.insert(2, 3, second);

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
        Quadtree<int> root/*(0, 0, 4, 4)*/;

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
    {
        printf("\nCache search on empty tree\n");
        int first = 'fi';
        int second = 'se';
        Quadtree<int> root;

        assert(root.search(1, 1) == NULL);
        assert(root.search(2, 1) == NULL);
        assert(root.cache_search(1, 1, [=](int x, int y) { return first; }) != NULL);
        assert(root.search(1, 1) != NULL);
        root.cache_search(2, 1, [=](int x, int y) { return second; });
        assert(root.search(2, 1) != NULL);
        assert(root.search(1, 2) == NULL);
        assert(root.search(1, 1)->get_data() == first);
        assert(root.search(2, 1)->get_data() == second);
    }
    {
        printf("\nCache search on empty tree with unique_ptr\n");
        int first = 'fi';
        int second = 'se';
        Quadtree<std::unique_ptr<int>> root;

        assert(root.search(1, 1) == NULL);
        assert(root.search(2, 1) == NULL);
        root.cache_search(1, 1, [=] (int x, int y) { return std::make_unique<int>(first); });
        assert(root.search(1, 1) != NULL);
        root.cache_search(2, 1, [=] (int x, int y) { return std::make_unique<int>(second); });
        assert(root.search(2, 1) != NULL);
        assert(root.search(1, 2) == NULL);
        assert(*root.search(1, 1)->get_data().get() == first);
        assert(*root.search(2, 1)->get_data().get() == second);
    }
    {
        printf("\nCache search with a class\n");
        struct S {
            int i;
        } first = {'fi'}, second = {'se'};
        Quadtree<S> root;

        assert(root.search(1, 1) == NULL);
        assert(root.search(2, 1) == NULL);
        root.cache_search(1, 1, [=](int x, int y) { return first; });
        assert(root.search(1, 1) != NULL);
        root.cache_search(2, 1, [=](int x, int y) { return second; });
        assert(root.search(2, 1) != NULL);
        assert(root.search(1, 2) == NULL);
        assert(root.search(1, 1)->get_data().i == first.i);
        assert(root.search(2, 1)->get_data().i == second.i);
    }
    return EXIT_SUCCESS;
}
