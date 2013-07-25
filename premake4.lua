solution "bsp"
   configurations { "Debug", "Release" }
   location "build/" 
   -- A project defines one build target
   project "bsp"
      kind "WindowedApp"
      language "C++"
      files { "src/**.h", "src/**.cpp", "src/**.c" }
      includedirs {"sdl/include/", "glew/glew-1.9.0/include", "bullet-2.81/src" } 
      defines {"GLEW_STATIC"} 
      
      libdirs {"sdl/VisualC/Release/", "sdl/VisualC/SDLmain/Release/", "bullet-2.81/lib/"}

      links {"SDL", "SDLmain" }

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