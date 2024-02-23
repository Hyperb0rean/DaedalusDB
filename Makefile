TARGET=database

configure:
	export CXX=/usr/bin/clang++-16 && cmake -S . -B build

configure-asan:
	export CXX=/usr/bin/clang++-16 && cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=ASAN

configure-release:
	export CXX=/usr/bin/clang++-16 && cmake -S . -B build-release -DCMAKE_BUILD_TYPE=release

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

clean:
	rm -rf build
	rm -rf build-asan
	rm -rf build-release
	rm -rf .cache
	rm -rf compile_commands.json
