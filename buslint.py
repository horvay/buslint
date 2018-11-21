#!/usr/bin/env python
# encoding: utf-8

#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

from waflib.TaskGen import feature, after_method, before_method
from waflib import Task, Logs, Utils, Errors
from waflib.Context import BOTH
import time
import tempfile
import os

@feature('buslint')
@after_method('process_source')
def create_buslint_tasks(self):
    # Skip during project generation
    if self.bld.env['PLATFORM'] == 'project_generator':
        return

    # don't bother to try this on non-windows
    host = Utils.unversioned_sys_platform()
    if host not in ('win_x64', 'win32'):
        return

    componentHeaders = [s for s in self.header_files if "Component.h" in str(s)]

    binary = self.bld.path.abspath() + "\\buslint.exe"
    inputs = self.to_nodes(self.source) + self.to_nodes(self.header_files)

    task = self.create_task('buslint', inputs)
    task.prepare(binary=binary, input_paths=' '.join(x.abspath() for x in componentHeaders))

class buslint(Task.Task):
    color = 'CYAN'

    def __init__(self, *k, **kw):
        super(buslint, self).__init__(*k, **kw)

    def prepare(self, *k, **kw):
        self.binary = kw['binary']
        self.input_paths = kw['input_paths']

    def run(self):
        args = self.input_paths

        # args might be more than 32k characters long, so let's write to a file and pass it to buslist
        f = tempfile.NamedTemporaryFile(delete=False)
        f.write(args)
        f.close()

        start_time = time.time()
        success = self.exec_buslint(f.name)
        elapsed_time = time.time() - start_time
        print("Buslint took " + str(elapsed_time) + "s to run! ")

        os.unlink(f.name)

        return 0 if success else 1

    def exec_buslint(self, args):
        """
        Execute the buslint with argument string
        :return: True on success, False on failure
        """
        command = '\"' + self.binary + '\" ' + args
        # print('buslint: Invoking buslint with command: {}'.format(command))

        try:
            (output, error_output) = self.generator.bld.cmd_and_log(command, output=BOTH, quiet=BOTH)
            if error_output and not error_output.isspace():
                Logs.error('buslint: buslint output to stderr even though it indicated success. Output:\n{}\nErrors:\n{}'.format(output, error_output))
                return False
        except Errors.WafError as e:
            Logs.error('buslint: buslint execution failed! Output:\n{}'.format(e.stderr))
            return False # do not fail the build, this is a warning

        return True
