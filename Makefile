CXXFLAGS+=-std=c++14 -Wall -pedantic -Wno-multichar -g
#CXXFLAGS+=-Og
quadtree: quadtree.cc quadtree.h
	$(CXX) $< $(CXXFLAGS) -o $@
