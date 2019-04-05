LB=tinyxml2/tinyxml2.cpp utfcpp/source/utf8/checked.h utfcpp/source/utf8/unchecked.h utfcpp/source/utf8/core.h 
all:
	g++ main.cpp ${LB} -o a.out
run:
	g++ main.cpp ${LB} -o a.out
	./a.out