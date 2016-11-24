#
# Copyright (c) 2015 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#

import os
import yin_cps
import yin_utils
import tempfile
import cps_c_lang
import cms_lang
import sys
import shutil
import argparse


class CPSYinFiles:
    yin_map = dict()
    yin_parsed_map = dict()

    def __init__(self, context):
        self.tmpdir = tempfile.mkdtemp()
        self.context = context

    def get_yin_file(self, filename):
        yin_file = os.path.join(
            self.tmpdir,
            os.path.splitext(os.path.basename(filename))[0] + ".yin")
        if not os.path.exists(yin_file):
            yin_utils.create_yin_file(filename, yin_file)
        return yin_file

    def get_parsed_yin(self, filename, prefix):
        key_name = os.path.splitext(filename)[0]
        key_name = os.path.split(key_name)[1]

        yin_key = key_name
        if prefix is not None:
            yin_key += ":" + prefix

        if yin_key not in self.yin_map:
            f = self.get_yin_file(filename)
            self.yin_map[yin_key] = yin_cps.CPSParser(self.context, f)
            _cps_parser = self.yin_map[yin_key]            
            _cps_parser.load(prefix=prefix)
            _cps_parser.walk()

            self.context['model-names'][_cps_parser.module.name()] = yin_key
            self.context['model-names'][_cps_parser.module.name()+'_model_'] = _cps_parser
            
        return self.yin_map[yin_key]

    def check_deps_loaded(self, module, context):
        entry = self.yin_map[module]
        for i in entry.imports:
            i = os.path.splitext(i)[0]
            if not i in context['current_depends']:
                return False
        return True

    def load_depends(self, module, context):
        entry = self.yin_map[module]
        for i in entry.imports:
            i = os.path.splitext(i)[0]
            if i in context['current_depends']:
                continue
            if not self.check_deps_loaded(i, context):
                self.load_depends(i, context)
            if i in context['current_depends']:
                raise Exception("")
            context['current_depends'].append(i)

        if module in context['current_depends']:
            return

        context['current_depends'].append(module)

    def load(self, yang_file, prefix=None):
        """Convert the yang file to a yin file and load the model"""
        return self.get_parsed_yin(yang_file, prefix)

    def seed(self, filename):
        s = set()
        l = list()
        l.append(filename)
        while len(l) > 0:
            f = l.pop()
            p = self.get_parsed_yin(f)
            for n in p.imports:
                if n not in l:
                    l.append(n)
        # parse based on dependencies
        context = dict()
        context['current_depends'] = list()
        for i in self.yin_map.keys():
            self.load_depends(i, context)

        print context['current_depends']

        for i in context['current_depends']:
            self.yin_map[i].parse()

    def cleanup(self):
        shutil.rmtree(self.tmpdir)



class CPSYangModel:
    model = None
    coutput = None
                
    def _default_ctx(self):
                        
        context = dict()
        context['output'] = {}
        context['output']['header'] = {}
        context['output']['src'] = {}        
        context['history'] = {}
        context['identity'] = {}
        context['types'] = {}
        context['enum'] = {}
        context['union'] = {}
        context['model-names'] = {}
        return context
               
    def __init__(self, args):                
        self.filename = args['file']
        self.context = self._default_ctx()
        
        ctx = self.context
        ctx['args'] = args
        
        _modules_ = { 'cps' : cps_c_lang.Language(ctx), 'cms' : cms_lang.Language(ctx) }
        
        ctx['output']['language'] = _modules_ #not really a language.. more of output plugins
        ctx['history']['output'] = ctx['args']['history']
        ctx['file-finder'] = CPSYinFiles(ctx)
        
        #delete next cleanup  
        ctx['loader'] = self.context['file-finder']

        self.model = ctx['file-finder'].load(self.filename)
        
        #assume order independent modules
        for i in _modules_.values():
            i.setup(self.model)

        for plugin in ctx['args']['output'].split(','):
            if plugin in _modules_:
                _modules_[plugin].write()
            

    def write_details(self, key):
        class_type = self.context['output'][key]
        if key in self.args:
            old_stdout = sys.stdout
            with open(self.args[key], "w") as sys.stdout:
                class_type.COutputFormat(self.context).show(self.model)
            sys.stdout = old_stdout

    def close(self):
        for i in self.context['output']['language']:
            self.context['output']['language'][i].close()

        self.context['loader'].cleanup()
