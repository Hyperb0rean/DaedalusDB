TARGET=database
COMPILER=/usr/bin/clang++-18 

.PHONY: clean configure configure-asan configure-release compile compile-asan compile-release test diff

configure:
	export CXX=$(COMPILER) && cmake -S  . -B build

configure-asan:
	export CXX=$(COMPILER) && cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=ASAN

configure-release:
	export CXX=$(COMPILER) && cmake -S . -B build-release -DCMAKE_BUILD_TYPE=release

compile: configure
	cmake --build build
	cp build/compile_commands.json compile_commands.json

compile-asan: configure-asan
	cmake --build build-asan
	cp build/compile_commands.json compile_commands.json

compile-release: configure-release
	cmake --build build-release
	cp build/compile_commands.json compile_commands.json

test: 
	./build$(TYPE)/$(TARGET)-test --gtest_filter="$(TEST).*"

diff:
	git diff --stat 2f47bd9c4758c96e2479a78f26dc9071bd4f365b HEAD -- ':!LICENSE'

clean:
	rm -rf build
	rm -rf build-asan
	rm -rf build-release
	rm -rf .cache
	rm -rf compile_commands.json
	rm -rf test.data
