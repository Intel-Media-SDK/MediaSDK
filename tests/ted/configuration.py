# -*- coding: utf-8 -*-

# Copyright (c) 2017-2020 Intel Corporation
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
import hashlib
import platform
import subprocess
import collections


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
    "av1",
    "jpg",
}

_RAW = {
    "i420",
}

_SUPPORTED_CODECS = _ENCODED | _RAW


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
            raise TestEnvironmentError("{} does not exist in {}".format(self.path.name, self.path))

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
    def __init__(self, cfg, base_dir):
        self.streams = {}

        if 'streams' not in cfg:
            raise ConfigurationError("No streams defined")
        try:
            folder = base_dir / 'content'
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


    def stream_by_name(self, name):
        try:
            return self.streams[name]
        except KeyError as ex:
            raise TestEnvironmentError("unknown stream - {}".format(name))


