"""
Script to configure native platform builds for unit testing.
Handles source file filtering and mock setup.
"""

Import("env")
import os

# For native platform tests, we only want to compile model.cpp and datetime.cpp
if env.get("PIOPLATFORM") == "native":
    # Get the source files to compile
    src_base = env.Dir("src")
    
    # Add the src directory to include path
    env.Prepend(CPPPATH=["src", "test/mocks"])
    
    # Compile the specific source files
    model_src = env.Dir("src").File("model.cpp")
    datetime_src = env.Dir("src").File("datetime.cpp")
    
    # Create object files from these sources
    model_obj = env.Object("$BUILD_DIR/src/model.o", model_src)
    datetime_obj = env.Object("$BUILD_DIR/src/datetime.o", datetime_src)
    
    # Add objects to be linked
    env.Prepend(OBJSUFFIX=[model_obj, datetime_obj])


