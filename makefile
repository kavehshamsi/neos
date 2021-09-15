CC = gcc
FLAGS = -O3 -g0 -fopenmp
CC_FLAGS = $(FLAGS)
CXX = g++-9
CXX_FLAGS = -w $(FLAGS) -std=c++2a #-DUSE_GAUSS=OFF 

BISON = bison
FLEX = flex

# static linking is not worth the pain
STATIC_EXEC = false

ifeq ($(strip $(STATIC_EXEC)), true)
	CXX_FLAGS += -static-libgcc -static-libstdc++	
endif

# build libneos.a
BUILD_STATICLIB = true

SAT_SOLVER = glucose # options : minisat, glucose

# Final binary
BIN = neos
TESTBIN = test_neos
SLIB = libneos.a
# Put all auto generated stuff to this build dir.
BUILD_DIR = ./build
BIN_DIR = ./bin
SOURCE_DIR = ./src

# List of all .cpp source files.
CPP_FILES = $(wildcard ./src/*/*.cpp)
CPP_FILES += $(wildcard ./src/*/*/*.cpp)

YY_FILES = $(wildcard ./src/parser/*/*.yy)
LL_FILES = $(wildcard ./src/parser/*/*.ll)

YY_CCFILES = $(YY_FILES:%.yy=%.tab.cc)
YY_HHFILES = $(YY_FILES:%.yy=%.tab.hh)
YY_ALLFILES = $(YY_CCFILES) $(YY_HHFILES) 
LL_CCFILES = $(LL_FILES:%.ll=%.tab.cc)

ifeq ($(strip $(SAT_SOLVER)),minisat)  
CPP_FILES += $(wildcard ./lib/minisat/*/*.cpp)
CPP_FILESCC += $(wildcard ./lib/minisat/*/*.cc)
endif

ifeq ($(strip $(SAT_SOLVER)),glucose)
CPP_FILES += $(wildcard ./lib/glucose/*/*.cpp)
endif

INCLUDES = -I./lib -I./src
INCLUDES += -I./lib/cudd -I./lib/cudd/cudd -I./lib/cudd/cplusplus -I./lib/cudd/epd/ -I./lib/cudd/mtr -I./lib/cudd/st

ifeq ($(strip $(SAT_SOLVER)),minisat)
	INCLUDES += -I./lib/minisat
endif

ifeq ($(strip $(SAT_SOLVER)),glucose)
	INCLUDES += -I./lib/glucose
endif


#LIBINCLUDES = -L./lib/cryptominisat/build/lib -R `pwd`/lib/cryptominisat/build/lib
LIBINCLUDES = -L/usr/local/lib/
STATICLIBS += ./lib/cudd/cudd/.libs/libcudd.a 
ARSTATICLIBS += ./lib/cudd/cudd/.libs/libcudd.a 

ifeq ($(strip $(STATIC_EXEC)), true)
	LDFLAGS += /usr/lib/x86_64-linux-gnu/libm.a /usr/lib/x86_64-linux-gnu/libpthread.a /usr/lib/x86_64-linux-gnu/libc.a \
	/usr/lib/x86_64-linux-gnu/libboost_program_options.a /usr/lib/x86_64-linux-gnu/libreadline.a \
	 /usr/lib/x86_64-linux-gnu/libhistory.a /usr/lib/x86_64-linux-gnu/libncurses.a /usr/lib/x86_64-linux-gnu/libtermcap.a
else
	LDFLAGS += -lpthread -lreadline -lboost_program_options
endif  

# All .o files go to build dir.
OBJ += $(CPP_FILES:%.cpp=$(BUILD_DIR)/%.o)
OBJ += $(CC_FILES:%.cc=$(BUILD_DIR)/%.o)
OBJ += $(C_FILES:%.c=$(BUILD_DIR)/%.o)
OBJ += $(YY_CCFILES:%.tab.cc=$(BUILD_DIR)/%.o)
OBJ += $(LL_CCFILES:%.tab.cc=$(BUILD_DIR)/%.o)

MAINOBJ = $(wildcard ./build/./src/main/*)
NONMAINOBJ = $(filter-out $(MAINOBJ), $(OBJ))
TESTCPP = $(wildcard ./tests/*.cpp)
TESTOBJ = $(filter-out $(MAINOBJ), $(OBJ))
TESTOBJ += $(TESTCPP:%.cpp=$(BUILD_DIR)/%.o)

# Gcc/Clang will create these .d files containing dependencies.
DEP = $(OBJ:%.o=%.d)
DEP += $(TESTOBJ:%.o=%.d)

# print target
print-%  : ; @echo $* = $($*)

# all target
all : neos

# Default target named after the binary.
neos : parser cudd $(BIN_DIR)/$(BIN)

# Actual target of the binary - depends on all .o files.
$(BIN_DIR)/$(BIN) : $(OBJ) cudd
# Create build directories - same structure as sources.
	mkdir -p $(@D)
# Just link all the object files.
#$(CXX) $(CXX_FLAGS) $(LIBINCLUDES) $(LDFLAGS) $(INCLUDES) $(STATICLIBS) $^ -o $@
	$(CXX) $(CXX_FLAGS) $(LIBINCLUDES) $(OBJ) $(INCLUDES) $(STATICLIBS) $(LDFLAGS) -o $@   
ifeq ($(BUILD_STATICLIB), true)
	ar -rc $(BIN_DIR)/$(SLIB) $(NONMAINOBJ)
endif

.PHONY: parser parser-clean

parser : $(YY_CCFILES) $(LL_CCFILES) $(YY_HHFILES) $(LL_FILES) $(YY_FILES)
	@echo 'building parsers $(YY_FILES) $(LL_FILES) done'

parser-clean :
	-rm $(YY_CCFILES) $(YY_HHFILES) $(LL_CCFILES)
	
%.tab.cc %.tab.hh : %.yy 
	$(BISON) -Wall -d -o $@ $<
	
%.tab.cc : %.ll
	$(FLEX) -o $@ $< 

# Include all .d files
-include $(DEP)

# Build target for every single object file.
# The potential dependency on header files is covered
# by calling `-include $(DEP)`.
	  
$(BUILD_DIR)/%.o : %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(VARS) -MMD -c $< -o $@

$(BUILD_DIR)/%.o : %.cc
	mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(VARS) -MMD -c $< -o $@

$(BUILD_DIR)/%.o : %.tab.cc
	mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(INCLUDES) $(VARS) -MMD -c $< -o $@

$(BUILD_DIR)/%.o : %.c
	mkdir -p $(@D)
# The -MMD flags additionaly creates a .d file with
# the same name as the .o file.
	$(CC) $(CC_FLAGS) $(INCLUDES) $(VARS) -MMD -c $< -o $@

$(TESTBIN) : $(BUILD_DIR)/$(TESTBIN)

$(BUILD_DIR)/$(TESTBIN) : $(TESTOBJ) cudd
# Create build directories - same structure as sources.
	mkdir -p $(@D)
# Just link all the object files.
	$(CXX) $(CXX_FLAGS) $(LIBINCLUDES) $(TESTOBJ) $(INCLUDES) $(STATICLIBS) $(LDFLAGS) -o $@

# for the cudd library build
cudd :
	cd lib/cudd/ && \
		make
	
	
.PHONY : clean
clean : parser-clean
# This should remove all generated files.
	-rm $(BUILD_DIR)/$(BIN) $(OBJ) $(DEP)

