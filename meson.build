project('dendrite', 'cpp')

sources = [
  'src/main.cpp', 'src/mainwindow.cpp', 'src/controller/controlpoint.cpp', 'src/controller/node.cpp',
  'src/controller/path.cpp', 'src/controller/selection.cpp', 'src/controller/sketch.cpp', 'src/controller/undo.cpp',
  'src/model/document.cpp', 'src/model/reference.cpp', 'src/model/sketch.cpp', 'src/serialisation/layout.cpp',
  'src/serialisation/reader.cpp', 'src/serialisation/writer.cpp', 'src/utilities/geometry.cpp', 'src/view/sketch.cpp',
]

cairo = dependency('cairo', version: '>= 1.18.0')
sigcpp = dependency('sigc++-3.0')
wxwidgets = dependency('wxwidgets', version: '>= 3.2.0', modules: [ 'std' ])
src = include_directories('src')

if build_machine.system() == 'windows'
  # reduce namespace pollution by windows.h
  add_project_arguments('-DNOGDI', language: 'cpp')

  windows = import('windows')
  sources += windows.compile_resources('platforms/windows/resources.rc',
    include_directories: wxwidgets.get_variable('rescomp').split()[2])
endif

executable('dendrite',
  sources: sources,
  dependencies: [ cairo, sigcpp, wxwidgets ],
  include_directories: src)
