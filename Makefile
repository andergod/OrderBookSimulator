dependency:
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. --graphviz=graph.dot && dot -Tpng graph.dot -o graphImage.png
prepare:
	rm -rf build
	mkdir build
