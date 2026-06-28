
set(PROGRAM_NAME WindEffectsEditor)

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.hpp")

add_executable(${{PROGRAM_NAME}} ${{SOURCE_FILES}})

# Link modules
target_link_libraries(${{PROGRAM_NAME}} PRIVATE 
    WindEffects-Core
    WindEffects-Application
)
