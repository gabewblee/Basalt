CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -I./src
LDLIBS = -lgtest -lgtest_main -lpthread

SRCS = src/buf/ExtendibleBufferPool.cpp src/buf/LRU.cpp src/filter/BloomFilter.cpp src/memtable/Memtable.cpp src/storage/BPlusTree.cpp src/disk/Writer.cpp src/database/Basalt.cpp libs/xxhash.c
TESTS = build/TestBPlusTree build/TestWriter build/TestBasalt build/TestSystem build/TestBloomFilter build/TestMemtable build/TestExtendibleBufferPool

all: build/Basalt $(TESTS)

cli: build/Basalt
	./build/Basalt

build:
	mkdir -p build

build/Basalt: src/Main.cpp $(SRCS) | build
	$(CXX) $(CXXFLAGS) src/Main.cpp $(SRCS) -o build/Basalt

build/TestBPlusTree: tests/TestBPlusTree.cpp $(SRCS) | build
	$(CXX) $(CXXFLAGS) tests/TestBPlusTree.cpp $(SRCS) -o build/TestBPlusTree $(LDLIBS)

build/TestWriter: tests/TestWriter.cpp $(SRCS) | build
	$(CXX) $(CXXFLAGS) tests/TestWriter.cpp $(SRCS) -o build/TestWriter $(LDLIBS)

build/TestBasalt: tests/TestBasalt.cpp $(SRCS) | build
	$(CXX) $(CXXFLAGS) tests/TestBasalt.cpp $(SRCS) -o build/TestBasalt $(LDLIBS)

build/TestSystem: tests/TestSystem.cpp $(SRCS) | build
	$(CXX) $(CXXFLAGS) tests/TestSystem.cpp $(SRCS) -o build/TestSystem $(LDLIBS)

build/TestBloomFilter: tests/TestBloomFilter.cpp src/filter/BloomFilter.cpp libs/xxhash.c | build
	$(CXX) $(CXXFLAGS) tests/TestBloomFilter.cpp src/filter/BloomFilter.cpp libs/xxhash.c -o build/TestBloomFilter $(LDLIBS)

build/TestMemtable: tests/TestMemtable.cpp src/memtable/Memtable.cpp | build
	$(CXX) $(CXXFLAGS) tests/TestMemtable.cpp src/memtable/Memtable.cpp -o build/TestMemtable $(LDLIBS)

build/TestExtendibleBufferPool: tests/TestExtendibleBufferPool.cpp src/buf/ExtendibleBufferPool.cpp src/buf/LRU.cpp libs/xxhash.c | build
	$(CXX) $(CXXFLAGS) tests/TestExtendibleBufferPool.cpp src/buf/ExtendibleBufferPool.cpp src/buf/LRU.cpp libs/xxhash.c -o build/TestExtendibleBufferPool $(LDLIBS)

test: all
	./build/TestBPlusTree
	./build/TestWriter
	./build/TestBasalt
	./build/TestSystem
	./build/TestBloomFilter
	./build/TestMemtable
	./build/TestExtendibleBufferPool

clean:
	rm -rf build

.PHONY: all cli test clean