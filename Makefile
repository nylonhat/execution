CXX := clang++
CEXT := cpp
CXXFLAGS := -O3  -g -Wall -std=c++26 #-flto -DNDEBUG  #-fsanitize=address #-ftemplate-depth=10000 #-fsanitize=thread

SRCPATH := ./src
TESTPATH := ./test
BINPATH := ./bin
OBJPATH := $(BINPATH)/obj
TESTOBJPATH := $(BINPATH)/testobj
LIBPATHS := ./dep/lib
LIBFLAGS :=
INCLUDEPATH := ./dep/include
MAKEDEPSPATH := ./etc/make-deps

EXE := program.exe

SRCS := $(wildcard $(SRCPATH)/*.$(CEXT))
TEST := $(wildcard $(TESTPATH)/*.$(CEXT))

OBJS := $(patsubst $(SRCPATH)/%.$(CEXT), $(OBJPATH)/%.o, $(SRCS)) 
TESTOBJS := $(patsubst $(TESTPATH)/%.$(CEXT), $(TESTOBJPATH)/%.o, $(TEST))

DEPENDS := $(patsubst $(SRCPATH)/%.$(CEXT), $(MAKEDEPSPATH)/%.d, $(SRCS)) 
TESTDEPENDS := $(patsubst $(TESTPATH)/%.$(CEXT), $(MAKEDEPSPATH)/%.d, $(TEST))


.PHONY: all run clean


all: $(EXE)

run:
	./$(EXE)

$(EXE): $(OBJS) $(TESTOBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ -L$(LIBPATHS) $(LIBFLAGS)

	
-include $(DEPENDS) $(TESTDEPENDS)

$(OBJPATH)/%.o: $(SRCPATH)/%.$(CEXT) Makefile | $(OBJPATH) $(MAKEDEPSPATH)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(MAKEDEPSPATH)/$*.d -I$(INCLUDEPATH) -c $< -o $@

$(TESTOBJPATH)/%.o: $(TESTPATH)/%.$(CEXT) Makefile | $(TESTOBJPATH) $(MAKEDEPSPATH)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(MAKEDEPSPATH)/$*.d -I$(INCLUDEPATH) -c $< -o $@
	
$(OBJPATH) $(TESTOBJPATH) $(MAKEDEPSPATH):
ifdef OS
	powershell.exe [void](New-Item -ItemType Directory -Path ./ -Name $@)
else
	mkdir -p $@
endif

clean:
ifdef OS
	powershell.exe if (Test-Path $(OBJPATH)) {Remove-Item $(OBJPATH) -Recurse}
	powershell.exe if (Test-Path $(TESTOBJPATH)) {Remove-Item $(TESTOBJPATH) -Recurse}
	powershell.exe if (Test-Path $(EXE)) {Remove-Item $(EXE)}
	powershell.exe if (Test-Path $(MAKEDEPSPATH)) {Remove-Item $(MAKEDEPSPATH) -Recurse}
else
	rm -rf $(OBJPATH)
	rm -rf $(TESTOBJPATH)
	rm -rf $(EXE)
	rm -rf $(MAKEDEPSPATH)
endif
