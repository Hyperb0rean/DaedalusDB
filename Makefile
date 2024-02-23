configure:
	export CXX=/usr/bin/clang++-16 && cmake -S . -B build

compile: configure
	cmake --build build
	cp build/compile_commands.json compile_commands.json


clean:
	rm -rf build
	rm -rf .cache
	rm -rf compile_commands.json
