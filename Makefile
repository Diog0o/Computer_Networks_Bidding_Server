CXX = g++
CXXFLAGS = -std=c++11

all: as user

as: as.cpp
	$(CXX) $(CXXFLAGS) -o as as.cpp

user: user.cpp
	$(CXX) $(CXXFLAGS) -o user user.cpp

clean:
	rm -f as user

