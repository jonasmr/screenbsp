solution "bsp"
   configurations { "Debug", "Release" }
   location "build/" 
   -- A project defines one build target
   project "bsp"
      kind "WindowedApp"
      language "C++"
      files { "src/**.h", "src/**.cpp", "src/**.c"}
      includedirs {"sdl2/include/", "glew/", "bullet-2.81/src" } 
      defines {"GLEW_STATIC"} 
      
      libdirs {"sdl2/VisualC/SDL/Win32/Release/", "sdl2/VisualC/SDLmain/Release/", "bullet-2.81/lib/"}

      links {"SDL2"}

      configuration "windows"
         links { "opengl32", "glu32", "winmm", "dxguid"}


      configuration "Debug"
         links {"BulletCollision_vs2010_debug", "BulletDynamics_vs2010_debug", "LinearMath_vs2010_debug"}         
         defines { "DEBUG" }
         flags { "Symbols", "StaticRuntime" }
 
      configuration "Release"
         links {"BulletCollision_vs2010", "BulletDynamics_vs2010", "LinearMath_vs2010"}         
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols", "StaticRuntime" }   

      configuration "SDL2.dll"
         buildaction "Copy"