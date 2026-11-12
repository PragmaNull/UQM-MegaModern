import os

from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import cmake_layout, CMakeDeps, CMakeToolchain


class UrQuanMastersConan(ConanFile):
    settings = 'os', 'arch', 'compiler', 'build_type'

    options = {
        'enable_asan': [True, False],
        'enable_linter': [True, False],
    }

    def configure(self):
        pass
        #if self.options.enable_asan and self.settings.build_type != 'Debug':
        #    raise ConanInvalidConfiguration('ASan requires Debug build')
    
    requires = (
        'asio/1.38.2',
        'cli11/2.6.0',
        'ogg/1.3.5',
        'fmt/12.1.0',
        'gsl-lite/1.0.1',
        'gtest/1.17.0',
        'imgui/1.92.8',
        'lua/5.4.7',
        'libmikmod/3.3.11.1',
        'libpng/1.6.54',
        'magic_enum/0.9.7',
        'nlohmann_json/3.12.0',
        'openal-soft/1.24.3',
        'scnlib/4.0.1',
        'sdl/3.4.16',
        'spdlog/1.17.0',
        'utf8proc/2.12.0',
        'vorbis/1.3.7',
        'yyjson/0.12.0',
        'zlib/1.3.1',
    )

    default_options = {
        'enable_asan': False,
        'enable_linter': False,
    }

    def requirements(self):
        # openal-soft and sdl require different pulseaudio versions on Linux.
        if self.settings.os == 'Linux':
            self.requires('pulseaudio/17.0', override=True)

    def layout(self):
        cmake_layout(self)
        
    def generate(self):
        tc = CMakeToolchain(self)
        # The imgui package has the platform and renderer backends only as sources.
        imgui_backends = os.path.join(self.generators_folder, 'imgui_backends')
        imgui_bindings = os.path.join(self.dependencies['imgui'].package_folder, 'res', 'bindings')
        for pattern in ('imgui_impl_sdl3.*', 'imgui_impl_sdlrenderer3.*'):
            copy(self, pattern, src=imgui_bindings, dst=imgui_backends)
        tc.variables['IMGUI_BACKENDS_DIR'] = imgui_backends.replace('\\', '/')
        tc.variables['ENABLE_ASAN'] = bool(self.options.enable_asan)
        tc.variables['ENABLE_LINTER'] = bool(self.options.enable_linter)
        tc.generate()

        # The utf8proc recipe expects 'utf8proc.lib' for static clang-cl builds.
        utf8proc = self.dependencies['utf8proc']
        if utf8proc.settings.get_safe('compiler.runtime') and not utf8proc.options.shared:
            utf8proc.cpp_info.libs = ['utf8proc_static']

        CMakeDeps(self).generate()