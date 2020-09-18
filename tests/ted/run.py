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
import re
import json
import pprint
import pathlib
import hashlib
import subprocess


class EncodedFileName(object):
    def __init__(self, case_id):
        self.case_id = case_id
        self.idx = 1
        self.names = []

    def __call__(self, m):
        filename = '{:04d}.{:02d}.out'.format(self.case_id, self.idx)
        self.names.append(filename)
        self.idx += 1

        return filename


def collect_md5(names, workdir, case_id):
    results = {}

    md5 = hashlib.md5()
    for name in names:
        with (workdir / name).open("rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                md5.update(chunk)

        results[name] = md5.hexdigest()

    result = workdir / "{:04d}.json".format(case_id)

    with result.open('w') as f:
        json.dump(results, f)

    return results


class Runner(object):
    def __init__(self, extra_env, cfg):
        self.env = os.environ.copy()
        if extra_env:
            self.env.update(extra_env)

        self.extra_env = extra_env
        self.cfg = cfg

    def _run(self, case_id, cmd, workdir, log):
        log.dump_header()
        log.separator()
        for var, val in self.extra_env.items():
            log.log("export {}={}".format(var, val))

        log.log("cd {}".format(workdir.resolve()))

        # ensure all parts of command line are strings
        cmd = [str(e) for e in cmd]
        log.log(subprocess.list2cmdline(cmd))
        log.separator()

        try:
            p = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=True,
                cwd=str(workdir),
                env=self.env,
                #shell=True
            )
            log.log(p.stdout.decode('utf-8'))
            log.separator()
            return p.returncode

        except subprocess.CalledProcessError as exc:
            log.log(exc.output.decode('utf-8'))
            log.separator()
            log.log("return code: {}".format(exc.returncode))

            return exc.returncode

    def other_options(self, case):
        cmd = []
        # process remaining arguments
        for k, v in case.items():
            if k in ['platforms']:
                continue
            if isinstance(v, bool):
                if v:
                    cmd.append("-{}".format(k))
            else:
                cmd.extend(["-{}".format(k), v])

        return cmd

    def sample_decode(self, case_id, case, workdir, log):
        cmd = ['sample_decode']
        cmd.append('-hw')

        stream = case.pop("stream")
        if stream.codec == "jpg":
            stream.codec = "jpeg"
        cmd.append(stream.codec)
        cmd.extend(['-i', str(stream.path)])

        decoded = "{:04d}.yuv".format(case_id)
        cmd.extend(['-o', decoded])

        cmd.extend(self.other_options(case))

        returncode = self._run(case_id, cmd, workdir, log)

        results = {}
        if returncode == 0:
            results = collect_md5([decoded], workdir, case_id)
            log.log(pprint.pformat(results))

        return results

    def sample_encode(self, case_id, case, workdir, log):
        cmd = ['sample_encode']

        encoder = case.pop('codec')
        cmd.append(encoder.codec)
        if encoder.plugin:
            cmd.extend(['-p', encoder.plugin['guid']])

        cmd.append('-hw')
        
        stream = case.pop('stream')
        cmd.extend(['-i', stream.path])
        cmd.extend(['-w', stream.width])
        cmd.extend(['-h', stream.height])

        if 'target_usage' in case:
            tu = case.pop('target_usage')
            cmd.extend(['-u', tu.usage])

        if 'quality' in case:
            quality = case.pop('quality')
            cmd.extend(['-q', quality])

        if 'bitrate' in case:
            bitrate = case.pop('bitrate')
            cmd.extend(['-b', bitrate])

        if 'qp' in case:
            qp = case.pop('qp')
            cmd.extend(['-cqp', '-qpi', qp, '-qpp', qp, '-qpb', qp])

        cmd.extend(self.other_options(case))

        encoded = "{:04d}.{}".format(case_id, encoder.codec)
        cmd.extend(['-o', encoded])

        returncode = self._run(case_id, cmd, workdir, log)

        results = {}
        if returncode == 0:
            results = collect_md5([encoded], workdir, case_id)
            log.log(pprint.pformat(results))

        return results

    def sample_multi_transcode(self, case_id, case, workdir, log):
        cmd = ['sample_multi_transcode']

        parfile = case.pop('parfile')
        encoded_fn = EncodedFileName(case_id)
        
        text = re.sub(r'\{out\}', encoded_fn, parfile.text)

        parfile = workdir / '{:04d}.par'.format(case_id)
        parfile.write_text(text)

        cmd.extend(['-par', parfile.name])

        returncode = self._run(case_id, cmd, workdir, log)

        results = {}
        if returncode == 0:
            results = collect_md5(encoded_fn.names, workdir, case_id)
            log.log(pprint.pformat(results))

        return results

    def sample_vpp(self, case_id, case, workdir, log):
        cmd = ['sample_vpp']

        cmd.extend(['-lib', 'hw'])

        stream = case.pop('stream')
        cmd.extend(['-i', stream.path])
        cmd.extend(['-sw', stream.width])
        cmd.extend(['-sh', stream.height])

        cmd.extend(self.other_options(case))
        
        processed = "{:04d}.vpp".format(case_id)

        cmd.extend(['-o', processed])
        returncode = self._run(case_id, cmd, workdir, log)

        results = {}
        if returncode == 0:
            results = collect_md5([processed], workdir, case_id)
            log.log(pprint.pformat(results))

        return results
