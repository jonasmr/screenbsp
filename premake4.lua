-- A solution contains projects, and defines the available configurations


solution "bsp"
   configurations { "Debug", "Release" }
   location "build/" 
   -- A project defines one build target
   project "bsp"
      kind "ConsoleApp"
      language "C++"
      files { "src/**.h", "src/**.cpp", "src/**.c" }
      includedirs {"sdl/include/", "glew/glew-1.9.0/include", "bullet-2.81/src" } 


      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }   