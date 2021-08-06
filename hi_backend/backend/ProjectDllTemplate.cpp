namespace hise { using namespace juce;

static const unsigned char projectDllTemplate_jucer_lines[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
"<JUCERPROJECT id=\"j2dKyn\" name=\"%NAME%\" projectType=\"dll\" jucerVersion=\"5.4.3\"\r\n"
"              cppLanguageStandard=\"17\">\r\n"
"  <MAINGROUP id=\"pGnv3y\" name=\"%NAME%\">\r\n"
"    <GROUP id=\"{35679809-0FA5-3027-50B5-CF866453D7B2}\" name=\"Source\">\r\n"
"      <FILE id=\"Sbp8Ar\" name=\"includes.h\" compile=\"0\" resource=\"0\" file=\"Source/includes.h\"/>\r\n"
"      <FILE id=\"LviiT0\" name=\"Main.cpp\" compile=\"1\" resource=\"0\" file=\"Source/Main.cpp\"/>\r\n"
"    </GROUP>\r\n"
"  </MAINGROUP>\r\n"
"  <EXPORTFORMATS>\r\n"
"    <VS2017 targetFolder=\"Builds/VisualStudio2017\" IPPLibrary=\"Sequential\">\r\n"
"      <CONFIGURATIONS>\r\n"
"        <CONFIGURATION isDebug=\"1\" name=\"Debug\" binaryPath=\"dll\" targetName=\"%DEBUG_DLL_NAME%\"/>\r\n"
"        <CONFIGURATION isDebug=\"0\" name=\"Release\" targetName=\"%RELEASE_DLL_NAME%\" binaryPath=\"dll\"\r\n"
"                       alwaysGenerateDebugSymbols=\"1\"/>\r\n"
"      </CONFIGURATIONS>\r\n"
"      <MODULEPATHS>\r\n"
"        <MODULEPATH id=\"juce_audio_basics\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_audio_formats\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_core\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_data_structures\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_events\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"hi_tools\" path=\"%HISE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"hi_dsp_library\" path=\"%HISE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_dsp\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_graphics\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_gui_basics\" path=\"%JUCE_PATH%\"/>\r\n"
"      </MODULEPATHS>\r\n"
"    </VS2017>\r\n"

"<XCODE_MAC targetFolder=\"Builds/MacOSX\" extraDefs=\"%USE_IPP_MAC%\" extraLinkerFlags=\"%IPP_COMPILER_FLAGS%\" xcodeValidArchs=\"x86_64\">\r\n"
"      <CONFIGURATIONS>\r\n"
"        <CONFIGURATION isDebug=\"1\" name=\"Debug\" osxArchitecture=\"64BitIntel\" headerPath=\"%IPP_HEADER%\"\r\n"
"                       libraryPath=\"%IPP_LIBRARY%\" binaryPath=\"dll\" targetName=\"%DEBUG_DLL_NAME%\"/>\r\n"
"        <CONFIGURATION isDebug=\"0\" name=\"Release\" osxArchitecture=\"64BitIntel\" headerPath=\"%IPP_HEADER%\"\r\n"
"                       libraryPath=\"%IPP_LIBRARY%\" binaryPath=\"dll\" targetName=\"%RELEASE_DLL_NAME%\"/>\r\n"
"      </CONFIGURATIONS>\r\n"
"      <MODULEPATHS>\r\n"
"        <MODULEPATH id=\"juce_gui_basics\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_graphics\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_events\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_dsp\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_data_structures\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_core\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_audio_formats\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_audio_basics\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"hi_tools\" path=\"%HISE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"hi_dsp_library\" path=\"%HISE_PATH%\"/>\r\n"
"      </MODULEPATHS>\r\n"
"    </XCODE_MAC>\r\n"
"<LINUX_MAKE targetFolder=\"Builds/LinuxMakefile\" extraCompilerFlags=\"-Wno-reorder -Wno-inconsistent-missing-override&#10;-fpermissive\"\r\n"
"                extraLinkerFlags=\"-no-pie&#10;\" linuxExtraPkgConfig=\"x11 xinerama xext\">\r\n"
"      <CONFIGURATIONS>\r\n"
"        <CONFIGURATION isDebug=\"1\" name=\"Debug\" binaryPath=\"dll\" targetName=\"project_debug\"/>\r\n"
"        <CONFIGURATION isDebug=\"0\" name=\"Release\" binaryPath=\"dll\" targetName=\"project\"/>\r\n"
"      </CONFIGURATIONS>\r\n"
"      <MODULEPATHS>\r\n"
"        <MODULEPATH id=\"juce_gui_basics\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_graphics\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_events\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_dsp\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_data_structures\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_core\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_audio_formats\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"juce_audio_basics\" path=\"%JUCE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"hi_tools\" path=\"%HISE_PATH%\"/>\r\n"
"        <MODULEPATH id=\"hi_dsp_library\" path=\"%HISE_PATH%\"/>\r\n"
"      </MODULEPATHS>\r\n"
"    </LINUX_MAKE>\r\n"
"  </EXPORTFORMATS>\r\n"
"  <MODULES>\r\n"
"    <MODULE id=\"hi_dsp_library\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"    <MODULE id=\"hi_tools\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"    <MODULE id=\"juce_audio_basics\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"    <MODULE id=\"juce_audio_formats\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"    <MODULE id=\"juce_core\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"    <MODULE id=\"juce_data_structures\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"    <MODULE id=\"juce_dsp\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"    <MODULE id=\"juce_events\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"    <MODULE id=\"juce_graphics\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"    <MODULE id=\"juce_gui_basics\" showAllCode=\"1\" useLocalCopy=\"0\" useGlobalPath=\"0\"/>\r\n"
"  </MODULES>\r\n"
"  <LIVE_SETTINGS>\r\n"
"    <WINDOWS/>\r\n"
"  </LIVE_SETTINGS>\r\n"
"  <JUCEOPTIONS HISE_NO_GUI_TOOLS=\"1\" HI_EXPORT_DSP_LIBRARY=\"0\" JUCE_LOAD_CURL_SYMBOLS_LAZILY=\"1\"\r\n"
"               IS_STATIC_DSP_LIBRARY=\"0\" HI_EXPORT_AS_PROJECT_DLL=\"1\"/>\r\n"
"</JUCERPROJECT>\r\n"
"\r\n";

const char* projectDllTemplate_jucer = (const char*)projectDllTemplate_jucer_lines;

}
