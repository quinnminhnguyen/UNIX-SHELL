all: Main

TemplateMain: Main.cpp
	c++ -std=c++14 Main.cpp -o Main

clean:
	rm *.o *.gch Main
