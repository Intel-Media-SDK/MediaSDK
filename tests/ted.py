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

import re
import sys
import json
import argparse
import collections
from pathlib import Path

from ted import discover

def write_test_results_report(cfg, test_results):
    results = collections.OrderedDict()
    results['system'] = collections.OrderedDict()
    results['system']['CPU'] = cfg.environment['CPU']
    results['system']['OS'] = cfg.environment['OS']

    with (base_dir / 'gold' / 'fileinfo.json').open() as f:
        gold_files = json.load(f)

    with (base_dir / 'results' / 'fileinfo.json').open() as f:
        current_files = json.load(f)

    results['bin'] = []
    for a, b in zip(gold_files, current_files):
        fileinfo = collections.OrderedDict()

        fileinfo['name'] = a['name']

        if 'version' in a:
            if a['version'] != b['version']:
                fileinfo['version'] = {
                    'ref': a['version'],
                    'cur': b['version']
                }
            else:
                fileinfo['version'] = a['version']

        if a['md5'] != b['md5']:
            fileinfo['md5'] = {
                'ref': a['md5'],
                'cur': b['md5']
            }
        else:
            fileinfo['md5'] = a['md5']

        if a['filesize'] != b['filesize']:
            fileinfo['filesize'] = {
                'ref': a['filesize'],
                'cur': b['filesize']
            }
        else:
            fileinfo['filesize'] = a['filesize']

        results['bin'].append(fileinfo)

    results['results'] = [res for res in test_results]

    test_results_json = base_dir / 'test_results.json'
    with test_results_json.open('w') as f:
        json.dump(results, f, indent=2)



if __name__ == '__main__':
    print('Intel(R) Media SDK Open Source TEst Driver')
    print('Copyright (c) Intel Corporation\n')

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--gold',
        action='store_true',
        default=False,
        help='collect gold results with vanila library'
    )

    parser.add_argument(
        'test',
        nargs='*',
        help='tests to run, if not specified all tests will be executed'
    )

    args = parser.parse_args()

    base_dir = Path(__file__).parent


    try:
        print("Setting up test environment...")
        cfg = discover.config(base_dir, args.gold)
    except Exception as ex:
        msg = "Can't load configuration - {}".format(ex)
        sys.exit(msg)

    test_re = None
    if args.test:
        test_re = re.compile('|'.join(args.test))

    tests_to_run = []
    print("Disovering tests...")
    for test in discover.tests(base_dir, cfg, args.gold):
        if test_re and not test_re.search(test.name):
            print('  {} - skipped'.format(test.name))
            continue
        if not args.gold and not test.gold_collected:
            print('  {} - no gold results collected'.format(test.name))
            continue

        tests_to_run.append(test)
        print('  {} - {} cases'.format(test.name, len(test.cases)))

    if not tests_to_run:
        sys.exit("Nothing to run")

    n = len(tests_to_run)
    if args.gold:
        print("\nCollecting gold results for {} test{}...".format(n, 's' if n > 1 else ''))
    else:
        print("\nRunning {} test{}...".format(n, 's' if n > 1 else ''))

    results = []
    total = passed = 0
    for test in tests_to_run:
        print('  {}'.format(test.name))
        if args.gold:
            total_, passed_ = test.mine()
        else:
            total_, passed_, details = test.run()

            results.append(details)


        total += total_
        passed += passed_

    print("\n{} of {} cases passed".format(passed, total))

    if not args.gold:
        write_test_results_report(cfg, results)

    # return code is number of failed cases
    sys.exit(total - passed)
