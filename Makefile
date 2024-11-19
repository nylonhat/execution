CXX := clang++
CEXT := cpp
CXXFLAGS := -O3  -g -Wall -std=c++23 -flto -DNDEBUG  #-fsanitize=address #-ftemplate-depth=10000 #-fsanitize=thread

SRCPATH := ./src
BINPATH := ./bin
OBJPATH := $(BINPATH)/obj
LIBPATHS := ./dep/lib
LIBFLAGS :=
INCLUDEPATH := ./dep/include
MAKEDEPSPATH := ./etc/make-deps

EXE := program.exe

SRCS := $(wildcard $(SRCPATH)/*.$(CEXT))
OBJS := $(patsubst $(SRCPATH)/%.$(CEXT), $(OBJPATH)/%.o, $(SRCS))

DEPENDS := $(patsubst $(SRCPATH)/%.$(CEXT), $(MAKEDEPSPATH)/%.d, $(SRCS))


.PHONY: all run clean


all: $(EXE)

run:
	./$(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ -L$(LIBPATHS) $(LIBFLAGS)

	
-include $(DEPENDS)

$(OBJPATH)/%.o: $(SRCPATH)/%.$(CEXT) Makefile | $(OBJPATH) $(MAKEDEPSPATH)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(MAKEDEPSPATH)/$*.d -I$(INCLUDEPATH) -c $< -o $@

	
$(OBJPATH) $(MAKEDEPSPATH):
ifdef OS
	powershell.exe [void](New-Item -ItemType Directory -Path ./ -Name $@)
else
	mkdir -p $@
endif

clean:
ifdef OS
	powershell.exe if (Test-Path $(OBJPATH)) {Remove-Item $(OBJPATH) -Recurse}
	powershell.exe if (Test-Path $(EXE)) {Remove-Item $(EXE)}
	powershell.exe if (Test-Path $(MAKEDEPSPATH)) {Remove-Item $(MAKEDEPSPATH) -Recurse}
else
	rm -r $(OBJPATH)
	rm -r $(EXE)
	rm -rf $(MAKEDEPSPATH)
endif
