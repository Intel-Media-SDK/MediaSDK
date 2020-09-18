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

import re
import pathlib


class ConfigurationError(Exception):
    pass


_BUILTIN_CODECS = {
    'h264',
    'mpeg2',
    'jpeg',
    'av1',
}

_PLUGIN_CODECS = {
    'h265',
    'vp8',
}


class Encoder(object):
    encode_plugin = {
        "h265": {
            "guid": "6fadc791a0c2eb479ab6dcd5ea9da347",
        }
    }

    def __init__(self, codec):
        if codec not in _BUILTIN_CODECS | _PLUGIN_CODECS:
            raise ConfigurationError("Unknown codec - {}".format(codec))

        self.plugin = None

        self.codec = codec
        if codec in self.encode_plugin:
            self.plugin = self.encode_plugin[codec]


class TargetUsage(object):
    def __init__(self, usage):
        if usage not in ('speed', 'quality', 'balanced'):
            raise ConfigurationError("Unknown target usage - {}".format(usage))

        self.usage = usage


class ParFile(object):
    def __init__(self, fn, base_dir, cfg):
        fn = base_dir / 'parfiles' / fn

        if not fn.exists():
            raise ConfigurationError("par-file '{}' does not exists".format(fn))

        text = fn.read_text()

        # replace input stream placeholders with full path to actual stream
        input_files = {}
        for m in re.finditer(r'\{([^\}]+)\}', text):
            
            if m.group(1) != 'out':
                input_files[m.group(0)] = cfg.stream_by_name(m.group(1))

        for fn, stream in input_files.items():
            text = text.replace(fn, str(stream.path))


        self.text = text
