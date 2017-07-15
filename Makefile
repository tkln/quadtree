CXXFLAGS+=-std=c++14 -Wall -pedantic -Wno-multichar -g

test: test.cc quadtree.h
	$(CXX) $< $(CXXFLAGS) -o $@
