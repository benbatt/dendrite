project('spline_draw', 'cpp')

gtkmm = dependency('gtkmm-4.0', version: '>= 4.10.0')
src = include_directories('src')

executable('spline_draw', 'src/main.cpp', 'src/mainwindow.cpp',
  'src/controller/controlpoint.cpp', 'src/controller/node.cpp', 'src/controller/path.cpp',
  'src/controller/sketch.cpp', 'src/controller/undo.cpp', 'src/model/sketch.cpp', 'src/utilities/geometry.cpp',
  'src/view/sketch.cpp',
  dependencies: gtkmm,
  include_directories: src)
