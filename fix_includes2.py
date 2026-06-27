import os

def replace_in_file(path, old, new):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    new_content = content.replace(old, new)
    if new_content != content:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print('Fixed:', path)

replace_in_file('f:/Coding/windeffects/Engine/Source/Editor/Legacy/Editor.cpp', '../Core/', 'Core/')
replace_in_file('f:/Coding/windeffects/Engine/Source/Editor/Legacy/ThumbnailManager.cpp', '../Renderer/', 'Renderer/')
replace_in_file('f:/Coding/windeffects/Engine/Source/Editor/Legacy/ThumbnailManager.hpp', '../Renderer/', 'Renderer/')
replace_in_file('f:/Coding/windeffects/Engine/Source/Editor/Legacy/Editor.hpp', '../Scene/', 'Scene/')
replace_in_file('f:/Coding/windeffects/Engine/Source/Editor/Legacy/Editor.hpp', '../Camera/', 'Camera/')
replace_in_file('f:/Coding/windeffects/Engine/Source/Editor/Legacy/Editor.hpp', '../Renderer/', 'Renderer/')
replace_in_file('f:/Coding/windeffects/Engine/Source/Programs/WindEffects/Main.cpp', 'Editor/Editor.hpp', 'Editor.hpp')

print("Done")
