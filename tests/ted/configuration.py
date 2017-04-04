# -*- coding: utf-8 -*-

# Copyright (c) 2017 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import os
import json
import shutil
import hashlib
import platform
import subprocess
import collections
import configparser


from pathlib import Path


class TestEnvironmentError(Exception):
    pass


class ConfigurationError(Exception):
    pass


_REQUIRED_SAMPLE_APPLICATIONS = [
    'sample_multi_transcode',
    'sample_decode',
    'sample_encode',
    'sample_vpp',
    'sample_fei',
]

_ENCODED = {
    "h264",
    "h265",
    "mpeg2",
    "vp8",
    "vp9",
    "jpg",
}

_RAW = {
    "i420",
}

_SUPPORTED_CODECS = _ENCODED | _RAW


def copy_file(src, dest):
    shutil.copy2(str(src), str(dest))


def collect_md5(fn):
    md5 = hashlib.md5()

    with fn.open("rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            md5.update(chunk)

    return md5.hexdigest()


def collect_file_info(fn, version=True):
    fileinfo = collections.OrderedDict()
    fileinfo['name'] = fn.name
    if version:
        try:
            p = subprocess.run(
                ['strings', str(fn)],
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
            )
            for line in p.stdout.splitlines():
                if line.startswith(b'mediasdk_file_version: '):
                    _, version = line.split(None, 1)
                    fileinfo['version'] = version.decode('utf-8', 'ignore')
                    break
        except subprocess.CalledProcessError:
            pass

    fileinfo['md5'] = collect_md5(fn)
    fileinfo['filesize'] = fn.stat().st_size

    return fileinfo


class Stream(object):
    def __init__(self, cfg, base_dir):
        self.path = base_dir / cfg['path']

        if not self.path.exists():
            raise TestEnvironmentError("{} does not exist".format(self.path.name))

        self.codec = cfg['codec']
        if self.codec not in _SUPPORTED_CODECS:
            raise ConfigurationError("unknown codec '{} for {}".format(
                self.codec,
                self.path.name
            ))

        self.width = int(cfg['width'])
        self.height = int(cfg['height'])

        self.frames = int(cfg['frames'])
        self.framerate = int(cfg['framerate'])


class Configuration(object):
    def __init__(self, cfg, base_dir, gold):
        self.streams = {}

        if 'streams' not in cfg:
            raise ConfigurationError("No streams defined")
        try:
            folder = cfg.get('streams_folder', r'$MFX_HOME/tests/content')
            streams_root = Path(os.path.expandvars(folder))

            for i, stream in enumerate(cfg['streams']):
                stream = Stream(stream, streams_root)
                self.streams[stream.path.name] = stream
        except KeyError as ex:
            raise ConfigurationError("required stream parameter {} is not set".format(ex))
        except ValueError as ex:
            raise ConfigurationError(ex)

        self.environment = collections.OrderedDict()

        self.environment['HOSTNAME'] = platform.node()
        self.environment['CPU'] = platform.processor()
        self.environment['OS'] = platform.platform()

        target = base_dir / ('gold' if gold else 'results')

        # prepare directory structure for test runs
        bins = target / 'bin'
        bins.mkdir(parents=True, exist_ok=True)
        libs = bins / 'lib'
        libs.mkdir(exist_ok=True)

        try:
            folder = cfg.get('samples_folder')
            self.mediasdk_samples = Path(os.path.expandvars(folder))
        except KeyError:
            raise ConfigurationError("'samples_folder' is not configured")

        try:
            folder = cfg.get('libmfx_folder')
            self.libmfx_folder = Path(os.path.expandvars(folder))
        except KeyError:
            raise ConfigurationError("'libmfx folder' is not configured")

        file_info = []

        libmfx = self.libmfx_folder / 'libmfxhw64.so'
        if not libmfx.exists():
            raise ConfigurationError("can't find 'libmfxhw64.so' in {}".format(self.libmfx_folder))

        copy_file(libmfx, libs)
        file_info.append(collect_file_info(libmfx))

        plugins_cfg = configparser.ConfigParser(interpolation=None)
        plugins_cfg.optionxform = str

        plugins_cfg.read(str(self.libmfx_folder.parents[1] / 'plugins.cfg'))
        for section in plugins_cfg.sections():
            plugin_cfg = configparser.ConfigParser()
            plugin_cfg.optionxform = str

            plugin_cfg.add_section(section)

            plugin_folder = None
            plugin_file = None

            for k, v in plugins_cfg.items(section):
                if k == 'GUID':
                    plugin_folder = bins / v
                    plugin_folder.mkdir(exist_ok=True)

                if k == 'Path':
                    k = 'FileName64'
                    plugin_file = v = str(Path(v).name)

                plugin_cfg[section][k] = v

            plugin = self.libmfx_folder / plugin_file
            if not plugin.exists():
                raise ConfigurationError("can't find '{}' in {}".format(
                    plugin.name, self.libmfx_folder
                ))

            copy_file(plugin, plugin_folder)

            file_info.append(collect_file_info(plugin))

            with (plugin_folder / 'plugin.cfg').open('w') as f:
                plugin_cfg.write(f)

        # copy samples to bin folder
        for app in _REQUIRED_SAMPLE_APPLICATIONS:
            if not (self.mediasdk_samples / app).exists():
                msg = "{} not found in {}".format(
                    app,
                    self.mediasdk_samples
                )
                raise TestEnvironmentError(msg)
            src = self.mediasdk_samples / app
            copy_file(src, bins)

            file_info.append(collect_file_info(src, version=False))

        with (target / "fileinfo.json").open('w') as f:
            json.dump(file_info, f, indent=4)

    def stream_by_name(self, name):
        try:
            return self.streams[name]
        except KeyError as ex:
            raise TestEnvironmentError("unknown stream - {}".format(name))


