EXECUTABLE := spline_draw.exe

all: $(EXECUTABLE)

SRC_DIR := ../src
OBJECTS := main.o mainwindow.o controller/node.o controller/sketch.o utilities/geometry.o view/sketch.o

CXXFLAGS += -std=c++17 $(shell pkg-config gtkmm-4.0 --cflags)
CXXFLAGS += -I../src
LDFLAGS += $(shell pkg-config gtkmm-4.0 --libs)

$(OBJECTS): %.o: ../src/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm $(OBJECTS) $(EXECUTABLE)
