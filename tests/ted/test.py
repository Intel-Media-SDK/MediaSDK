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
import datetime
import collections
import itertools

from pathlib import Path

from . import objects, run, platform


class ValidationError(Exception):
    pass


class CaseLogger(object):
    def __init__(self, fn, cfg):
        self.fn = fn
        self.cfg = cfg

    def log(self, msg):
        with self.fn.open('a+') as f:
            f.write(msg + '\n')

    def dump_header(self):
        for key, value in self.cfg.environment.items():
            self.log("{:>8s}: {}".format(key, value))

        self.log(" started: {}".format(datetime.datetime.now()))

    def separator(self):
        self.log('-' * 78)


class Test(object):
    def __init__(self, fn, base, cfg, args):
        self.cases = []
        self.cfg = cfg
        self.test_type = None
        self.base_dir = Path(base)
        self.device = args.device

        fn = Path(fn)

        self.config = json.loads(fn.read_text(), object_pairs_hook=collections.OrderedDict)
        self.name = fn.stem

        self.results = self.base_dir / 'results' / self.name

        self.generate_cases()

        self.runner = run.Runner(dict(), cfg)

    def clear_results(self):
        if self.results.exists():
            for fn in self.results.glob('*.*'):
                fn.unlink()

    def remove_generated(self, results, folder):
        for fn in results.keys():
            try:
                (folder / fn).unlink()
            except Exception:
                pass

    def exec_test_tool(self, case_id, params, workdir, log):
        if self.test_type == 'decode':
            return self.runner.sample_decode(case_id, params, workdir, log)
        elif self.test_type == 'encode':
            return self.runner.sample_encode(case_id, params, workdir, log)
        elif self.test_type == 'transcode':
            return self.runner.sample_multi_transcode(case_id, params, workdir, log)
        elif self.test_type == 'vpp':
            return self.runner.sample_vpp(case_id, params, workdir, log)

    def run(self):
        self.clear_results()
        self.results.mkdir(parents=True, exist_ok=True)
        total = passed = 0

        details = {
            'test': self.name,
            'cases': []
        }
        for i, case in enumerate(self.cases, 1):
            log = CaseLogger(self.results / "{:04d}.log".format(i), self.cfg)

            error = None

            total += 1
            print("    {:04d}".format(i), end="")
            results = self.exec_test_tool(i, case, self.results, log)

            if results:
                passed += 1
                log.log('PASS')
            else:
                error = "fail"
                log.log('FAIL')
            self.remove_generated(results, self.results)
            log.separator()
            res = {
                'id': '{:04d}'.format(i),
            }
            if error:
                print(' - FAIL')
                res['status'] = 'FAIL'
                # Print log in case of the failure
                with log.fn.open('r') as f:
                    print(f.read())
                res['error'] = error
                log.log(error)
                res['artifacts'] = results
            else:
                print(' - ok')
                log.log('PASS')
                res['status'] = 'PASS'
                res['artifacts'] = results

            details['cases'].append(res)

            log.log('\nfinisned: {}'.format(datetime.datetime.now()))

        return (total, passed, details)

    def generate_cases(self):
        self.platforms = self.config.get('platforms', None)
        device_name = Path(self.device).name
        if self.platforms is not None:
            if not platform.check_platform(platform.get_current_device_id(device_name), self.platforms):
                raise platform.UnsupportedPlatform("test is disabled for current platform")

        self.test_type = self.config.get('type', None)
        if self.test_type not in ['decode', 'encode', 'transcode', 'vpp']:
            raise ValidationError("unknown test type")

        # let's generate preliminary cases list
        keys = []
        values = []

        for key, val in self.config.items():
            if key == 'type':
                continue

            keys.append(key)
            if isinstance(val, list):
                values.append(val)
            else:
                values.append([val])

        for vals in itertools.product(*values):
            case = collections.OrderedDict(zip(keys, vals))
            case['device'] = self.device

            if 'stream' not in case:
                if self.test_type != 'transcode':
                    raise ValidationError("stream is not defined")
            else:
                # process common options
                case['stream'] = self.cfg.stream_by_name(case['stream'])

            if 'codec' in case:
                case['codec'] = objects.Encoder(case['codec'])
                if case['codec'].codec == 'jpeg':
                    if 'quality' not in case:
                        raise ValidationError("undefined JPEG quality")
                    if 'target_usage' in case:
                        raise ValidationError("JPEG encoder does not support target usage")
                    if 'bitrate' in case or 'qp' in case:
                        raise ValidationError("JPEG encoder does not support bitrate or QP setting")

                else:
                    if 'target_usage' in case:
                        case['target_usage'] = objects.TargetUsage(case['target_usage'])

                    if 'bitrate' not in case and 'qp' not in case:
                        raise ValidationError("undefined bitrate or QP")
                    if 'bitrate' in case and 'qp' in case:
                        raise ValidationError("both bitrate and QP defined")

            if 'parfile' in case:
                case['parfile'] = objects.ParFile(case['parfile'], self.base_dir, self.cfg)

            if self.test_type == 'transcode' and 'parfile' not in case:
                raise ValidationError("unknown parfile for transcode test")

            if self.test_type == 'encode' and 'codec' not in case:
                raise ValidationError("unknown codec for encode test")

            self.cases.append(case)

