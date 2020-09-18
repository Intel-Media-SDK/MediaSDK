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

import json
import pathlib

from . import test, configuration


def tests(base_dir, cfg, args):
    base_dir = pathlib.Path(base_dir)

    for fn in (base_dir / 'tests').rglob("*.json"):
        try:
            yield test.Test(fn, base_dir, cfg, args)
        except Exception as ex:
            print(" WARN: Can't parse test '{}' - {}".format(fn.name, ex))


def config(base_dir):
    fn = base_dir / 'ted.json'

    cfg = json.loads(fn.read_text())

    return configuration.Configuration(cfg, base_dir)


