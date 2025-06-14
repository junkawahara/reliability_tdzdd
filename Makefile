# Compiler flags
CXX = g++
CXXFLAGS = -I. -ISAPPOROBDD -DB_64 -O3 -Wall -Wextra -Wno-unused-parameter -std=c++11
PCH_FLAGS = -DUSE_PCH

# Precompiled header
PCH_FILE = pch.hpp
PCH_OUTPUT = pch.hpp.gch

# Object files
SAPPOROBDD_OBJ = SAPPOROBDD/BDD.o
VCONST_OP_OBJ = vconst_op.o

reliability: reliability.cpp $(VCONST_OP_OBJ) $(SAPPOROBDD_OBJ)
	$(CXX) $(VCONST_OP_OBJ) $(SAPPOROBDD_OBJ) reliability.cpp -o reliability $(CXXFLAGS)

reliability-pch: $(PCH_OUTPUT) reliability.cpp $(VCONST_OP_OBJ) $(SAPPOROBDD_OBJ)
	$(CXX) $(VCONST_OP_OBJ) $(SAPPOROBDD_OBJ) reliability.cpp -o reliability-pch $(CXXFLAGS) $(PCH_FLAGS)

reliability-confirm: reliability.cpp $(VCONST_OP_OBJ) $(SAPPOROBDD_OBJ)
	$(CXX) $(VCONST_OP_OBJ) $(SAPPOROBDD_OBJ) reliability.cpp -o reliability-confirm $(CXXFLAGS) -DINPUT_CONFIRM_MODE

reliability-confirm-pch: $(PCH_OUTPUT) reliability.cpp $(VCONST_OP_OBJ) $(SAPPOROBDD_OBJ)
	$(CXX) $(VCONST_OP_OBJ) $(SAPPOROBDD_OBJ) reliability.cpp -o reliability-confirm-pch $(CXXFLAGS) $(PCH_FLAGS) -DINPUT_CONFIRM_MODE

$(PCH_OUTPUT): $(PCH_FILE)
	$(CXX) $(CXXFLAGS) -c $(PCH_FILE) -o $(PCH_OUTPUT)

$(SAPPOROBDD_OBJ): SAPPOROBDD/BDD.cc
	$(CXX) -c SAPPOROBDD/BDD.cc -o $(SAPPOROBDD_OBJ) $(CXXFLAGS)

$(VCONST_OP_OBJ): vconst_op.cpp
	$(CXX) -c vconst_op.cpp -o $(VCONST_OP_OBJ) $(CXXFLAGS)

# Convenience targets
pch: reliability-pch
fast: reliability-pch
debug: reliability-confirm-pch

clean:
	rm -f reliability reliability-pch reliability-confirm reliability-confirm-pch
	rm -f $(VCONST_OP_OBJ) $(VCONST_OP_PCH_OBJ) $(SAPPOROBDD_OBJ) $(PCH_OUTPUT) *.o
