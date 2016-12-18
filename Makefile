CXXFLAGS+=-std=c++14 -Wall -pedantic -Wno-multichar
#CXXFLAGS+=-Og -g
quadtree: quadtree.cc quadtree.h
	$(CXX) $< $(CXXFLAGS) -o $@
