-- A solution contains projects, and defines the available configurations
solution "bsp"
   configurations { "Debug", "Release" }
 
   -- A project defines one build target
   project "bsp"
      kind "ConsoleApp"
      language "C++"
      files { "src/**.h", "src/**.cpp", "src/**.c" }

      location "build/"
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }   