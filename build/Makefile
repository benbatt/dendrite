EXECUTABLE := spline_draw.exe

all: $(EXECUTABLE)

SRC_DIR := ../src
OBJECTS := main.o mainwindow.o drawingarea.o

CXXFLAGS += -std=c++17 $(shell pkg-config gtkmm-4.0 --cflags)
LDFLAGS += $(shell pkg-config gtkmm-4.0 --libs)

$(OBJECTS): %.o: ../src/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm $(OBJECTS) $(EXECUTABLE)
