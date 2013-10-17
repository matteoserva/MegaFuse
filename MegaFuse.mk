##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=MegaFuse
ConfigurationName      :=Debug
WorkspacePath          := "/home/matteo/Progetti/CodeLite"
ProjectPath            := "/home/matteo/Progetti/MegaFuse"
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=matteo
Date                   :=17/10/2013
CodeLitePath           :="/home/matteo/.codelite"
LinkerName             :=g++
SharedObjectLinkerName :=g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.o.i
DebugSwitch            :=-gstab
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E 
ObjectsFileList        :="MegaFuse.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch)/usr/include/cryptopp $(IncludeSwitch)./inc $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)cryptopp $(LibrarySwitch)freeimage $(LibrarySwitch)curl $(LibrarySwitch)db_cxx $(LibrarySwitch)fuse 
ArLibs                 :=  "cryptopp" "freeimage" "curl" "db_cxx" "fuse" 
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := ar rcus
CXX      := g++
CC       := gcc
CXXFLAGS := -std=c++0x -D_FILE_OFFSET_BITS=64 -g -O0 -Wall $(Preprocessors)
CFLAGS   := -D_FILE_OFFSET_BITS=64 -g -O0 -Wall $(Preprocessors)


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/src_megacrypto$(ObjectSuffix) $(IntermediateDirectory)/src_megafusecallbacks$(ObjectSuffix) $(IntermediateDirectory)/src_fuseFileCache$(ObjectSuffix) $(IntermediateDirectory)/src_megaposix$(ObjectSuffix) $(IntermediateDirectory)/src_fuseImpl$(ObjectSuffix) $(IntermediateDirectory)/src_fuse$(ObjectSuffix) $(IntermediateDirectory)/src_megacli$(ObjectSuffix) $(IntermediateDirectory)/src_megabdb$(ObjectSuffix) $(IntermediateDirectory)/src_megaclient$(ObjectSuffix) $(IntermediateDirectory)/src_megafuse$(ObjectSuffix) \
	$(IntermediateDirectory)/src_Config$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/src_megacrypto$(ObjectSuffix): src/megacrypto.cpp $(IntermediateDirectory)/src_megacrypto$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/megacrypto.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_megacrypto$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_megacrypto$(DependSuffix): src/megacrypto.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_megacrypto$(ObjectSuffix) -MF$(IntermediateDirectory)/src_megacrypto$(DependSuffix) -MM "src/megacrypto.cpp"

$(IntermediateDirectory)/src_megacrypto$(PreprocessSuffix): src/megacrypto.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_megacrypto$(PreprocessSuffix) "src/megacrypto.cpp"

$(IntermediateDirectory)/src_megafusecallbacks$(ObjectSuffix): src/megafusecallbacks.cpp $(IntermediateDirectory)/src_megafusecallbacks$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/megafusecallbacks.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_megafusecallbacks$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_megafusecallbacks$(DependSuffix): src/megafusecallbacks.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_megafusecallbacks$(ObjectSuffix) -MF$(IntermediateDirectory)/src_megafusecallbacks$(DependSuffix) -MM "src/megafusecallbacks.cpp"

$(IntermediateDirectory)/src_megafusecallbacks$(PreprocessSuffix): src/megafusecallbacks.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_megafusecallbacks$(PreprocessSuffix) "src/megafusecallbacks.cpp"

$(IntermediateDirectory)/src_fuseFileCache$(ObjectSuffix): src/fuseFileCache.cpp $(IntermediateDirectory)/src_fuseFileCache$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/fuseFileCache.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_fuseFileCache$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_fuseFileCache$(DependSuffix): src/fuseFileCache.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_fuseFileCache$(ObjectSuffix) -MF$(IntermediateDirectory)/src_fuseFileCache$(DependSuffix) -MM "src/fuseFileCache.cpp"

$(IntermediateDirectory)/src_fuseFileCache$(PreprocessSuffix): src/fuseFileCache.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_fuseFileCache$(PreprocessSuffix) "src/fuseFileCache.cpp"

$(IntermediateDirectory)/src_megaposix$(ObjectSuffix): src/megaposix.cpp $(IntermediateDirectory)/src_megaposix$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/megaposix.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_megaposix$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_megaposix$(DependSuffix): src/megaposix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_megaposix$(ObjectSuffix) -MF$(IntermediateDirectory)/src_megaposix$(DependSuffix) -MM "src/megaposix.cpp"

$(IntermediateDirectory)/src_megaposix$(PreprocessSuffix): src/megaposix.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_megaposix$(PreprocessSuffix) "src/megaposix.cpp"

$(IntermediateDirectory)/src_fuseImpl$(ObjectSuffix): src/fuseImpl.cpp $(IntermediateDirectory)/src_fuseImpl$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/fuseImpl.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_fuseImpl$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_fuseImpl$(DependSuffix): src/fuseImpl.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_fuseImpl$(ObjectSuffix) -MF$(IntermediateDirectory)/src_fuseImpl$(DependSuffix) -MM "src/fuseImpl.cpp"

$(IntermediateDirectory)/src_fuseImpl$(PreprocessSuffix): src/fuseImpl.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_fuseImpl$(PreprocessSuffix) "src/fuseImpl.cpp"

$(IntermediateDirectory)/src_fuse$(ObjectSuffix): src/fuse.c $(IntermediateDirectory)/src_fuse$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/fuse.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_fuse$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_fuse$(DependSuffix): src/fuse.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_fuse$(ObjectSuffix) -MF$(IntermediateDirectory)/src_fuse$(DependSuffix) -MM "src/fuse.c"

$(IntermediateDirectory)/src_fuse$(PreprocessSuffix): src/fuse.c
	@$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_fuse$(PreprocessSuffix) "src/fuse.c"

$(IntermediateDirectory)/src_megacli$(ObjectSuffix): src/megacli.cpp $(IntermediateDirectory)/src_megacli$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/megacli.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_megacli$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_megacli$(DependSuffix): src/megacli.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_megacli$(ObjectSuffix) -MF$(IntermediateDirectory)/src_megacli$(DependSuffix) -MM "src/megacli.cpp"

$(IntermediateDirectory)/src_megacli$(PreprocessSuffix): src/megacli.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_megacli$(PreprocessSuffix) "src/megacli.cpp"

$(IntermediateDirectory)/src_megabdb$(ObjectSuffix): src/megabdb.cpp $(IntermediateDirectory)/src_megabdb$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/megabdb.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_megabdb$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_megabdb$(DependSuffix): src/megabdb.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_megabdb$(ObjectSuffix) -MF$(IntermediateDirectory)/src_megabdb$(DependSuffix) -MM "src/megabdb.cpp"

$(IntermediateDirectory)/src_megabdb$(PreprocessSuffix): src/megabdb.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_megabdb$(PreprocessSuffix) "src/megabdb.cpp"

$(IntermediateDirectory)/src_megaclient$(ObjectSuffix): src/megaclient.cpp $(IntermediateDirectory)/src_megaclient$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/megaclient.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_megaclient$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_megaclient$(DependSuffix): src/megaclient.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_megaclient$(ObjectSuffix) -MF$(IntermediateDirectory)/src_megaclient$(DependSuffix) -MM "src/megaclient.cpp"

$(IntermediateDirectory)/src_megaclient$(PreprocessSuffix): src/megaclient.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_megaclient$(PreprocessSuffix) "src/megaclient.cpp"

$(IntermediateDirectory)/src_megafuse$(ObjectSuffix): src/megafuse.cpp $(IntermediateDirectory)/src_megafuse$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/megafuse.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_megafuse$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_megafuse$(DependSuffix): src/megafuse.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_megafuse$(ObjectSuffix) -MF$(IntermediateDirectory)/src_megafuse$(DependSuffix) -MM "src/megafuse.cpp"

$(IntermediateDirectory)/src_megafuse$(PreprocessSuffix): src/megafuse.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_megafuse$(PreprocessSuffix) "src/megafuse.cpp"

$(IntermediateDirectory)/src_Config$(ObjectSuffix): src/Config.cpp $(IntermediateDirectory)/src_Config$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/matteo/Progetti/MegaFuse/src/Config.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_Config$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_Config$(DependSuffix): src/Config.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_Config$(ObjectSuffix) -MF$(IntermediateDirectory)/src_Config$(DependSuffix) -MM "src/Config.cpp"

$(IntermediateDirectory)/src_Config$(PreprocessSuffix): src/Config.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_Config$(PreprocessSuffix) "src/Config.cpp"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) $(IntermediateDirectory)/src_megacrypto$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_megacrypto$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_megacrypto$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_megafusecallbacks$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_megafusecallbacks$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_megafusecallbacks$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_fuseFileCache$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_fuseFileCache$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_fuseFileCache$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_megaposix$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_megaposix$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_megaposix$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_fuseImpl$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_fuseImpl$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_fuseImpl$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_fuse$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_fuse$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_fuse$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_megacli$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_megacli$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_megacli$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_megabdb$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_megabdb$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_megabdb$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_megaclient$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_megaclient$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_megaclient$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_megafuse$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_megafuse$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_megafuse$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/src_Config$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/src_Config$(DependSuffix)
	$(RM) $(IntermediateDirectory)/src_Config$(PreprocessSuffix)
	$(RM) $(OutputFile)
	$(RM) "../CodeLite/.build-debug/MegaFuse"


