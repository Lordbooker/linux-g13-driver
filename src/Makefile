EXE_NAME = Linux-G13-Driver
SRC_DIR  = cpp
OBJS     = $(SRC_DIR)/G13.o $(SRC_DIR)/G13Action.o $(SRC_DIR)/Macro.o $(SRC_DIR)/MacroAction.o $(SRC_DIR)/Main.o $(SRC_DIR)/Output.o $(SRC_DIR)/PassThroughAction.o
CC       = g++
# FLAGS    = -L /lib64 -l pthread  # Replaced by CXXFLAGS, LDFLAGS
# LIBS     = -lusb-1.0             # Replaced by LDLIBS

# Compiler and Linker Flags
CXXFLAGS = -pthread # Add other C++ compilation flags like -Wall, -O2, -std=c++11
CPPFLAGS =           # For preprocessor flags like -Iinclude/ -DDEBUG (e.g. -I../include if headers are in a parent include dir)
LDFLAGS  = -L/lib64
LDLIBS   = -lusb-1.0 # -pthread in CXXFLAGS, when passed to the linker, handles pthread linking

all: $(EXE_NAME)

# Pattern rule for compiling .cpp files from SRC_DIR to .o files in SRC_DIR
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(EXE_NAME): $(OBJS)
	$(CC) $(OBJS) -o $(EXE_NAME) $(LDFLAGS) $(CXXFLAGS) $(LDLIBS)

clean:
	rm -f $(OBJS) $(EXE_NAME)

