# -*- coding: utf-8 -*-

# Copyright (c) 2020 Intel Corporation
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

from . import objects

DEVICE_IDS = {
    "tgl": {
        0x9A40,
        0x9A49,
        0x9A59,
        0x9A60,
        0x9A68,
        0x9A70,
        0x9A78,
        0x9AC0,
        0x9AC9,
        0x9AD9,
        0x9AF8,
    }
}


class UnsupportedPlatform(objects.ConfigurationError):
    pass


def check_platform(current_device_id, supported_platforms):
    current_platform = '<unknown>'
    for platform, ids in DEVICE_IDS.items():
        if current_device_id in ids:
            current_platform = platform
    return current_platform in supported_platforms


def get_current_device_id(device_name):
    device_id_path = "/sys/class/drm/{}/device/device".format(device_name)
    if os.path.isfile(device_id_path):
        with open(device_id_path) as f:
            return int(f.read(), base=16)
