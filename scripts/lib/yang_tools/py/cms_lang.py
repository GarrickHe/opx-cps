#
# Copyright (c) 2015 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN #AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
#  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#

import sys
import os
import yin_utils

cma_gen_file_c_includes = """

#include \"cps_api_operation.h\"
#include \"cma_utilities.h\"
#include \"cma_init.h\"
#include \"cma_errnum.h\"


"""
write_statement_switch = """
  t_std_error retval = STD_ERR_OK;
  switch (edit_mode.phase) {
  case CMA_PH_APPLY:
     /* Check whether ready for commit */
     break;
  case CMA_PH_ROLLBACK:
     /* Code for undoing commit here */
     break;
  case CMA_PH_COMMIT:
     /* Commit phase code here */
     switch (edit_mode.op) {
     case CMA_OP_LOAD:
         /*This is called at module startup to reload all data that module owns*/
         break;
     case CMA_OP_CREATE:
         /*Called when module is created*/
         break;
     case CMA_OP_MERGE:
         break;
     case CMA_OP_DELETE:
         break;
     default:
         retval = CMA_ERR_OPERATION_NOT_SUPPORTED;
     }
     break;
  default:
     retval = CMA_ERR_OPERATION_FAILED;
 }
"""


class Language:

    supported_list_containing_cb = [
        "container", "grouping", "case", "list", "rpc", "choice", "augment"
    ]

    def __init__(self, context):
        self.context = context

    def change_name_for_augment(self, name):

        if self.all_node_map[name].get('augmented') ==True:
             ns = self.all_node_map[name].get('target-namespace')
             ns_index = str.find(name, "/" + ns + "/")
             if ns_index:
                  name1 = name[:ns_index]
                  name2= name[ns_index+len(ns)+1:]
                  name = name1+name2
                  return name


    def change_prefix(self, name, model):
        prefix = model.module.prefix
        loc = name.find(prefix)
        if loc != 0:
            return name
        name = model.module.module + name[len(prefix):]
        return name

    def name_to_cms_name(self, name):
        name = name.replace('/', '_')
        name = name.replace('-', '_')
        name = name.replace(':', '_')
        return name

    def setup_names(self):
        self.names = {}
        self.aug_names = {}
        for k in self.all_node_map:
            name = self.name_to_cms_name(self.change_prefix(k,self.model))
            self.names[k] = name
            self.names[name]=k
            aug_name = self.change_name_for_augment(k)
            if aug_name != None:
                aug_name = self.name_to_cms_name(self.change_prefix(aug_name,self.model))
            else:
                aug_name = name
            self.aug_names[k]=aug_name

        #go through the list of modules augmented and setup names for nodes
        if self.module_obj.get_if_augments() is True:
            for model in self.model.augment_list:
               for k in self.all_node_map:
                   if k is model.module.prefix:
                       continue
                   name = self.name_to_cms_name(self.change_prefix(k,model))
                   self.names[k] = name
                   self.names[name] = k


        for k in self.model.key_elements:
            for key_element in self.model.key_elements[k].split():
                if key_element in self.names: continue
                name = ''
                #get the name from the CPS model since we don't have a mapping here
                if key_element not in self.context['output']['language']['cps'].names:
                    continue

                name = self.context['output']['language']['cps'].names[key_element]

                self.names[key_element] = name
                self.names[name] = key_element

        self.names_short = {}
        seen = {}
        model_list = []
        model_list.append(self.model)
        for j in self.model.augment_list:
            model_list.append(j)

        for k in self.all_node_map:
            if k.rfind('/') != -1:
                name = k[k.rfind('/') + 1:]
            else:
                name = k
            to_add = self.name_to_cms_name(name)

            index = 0
            while to_add in seen:
                to_add = name + str(index)
                index += 1
            seen[to_add] = to_add

            name = to_add

            self.names_short[self.names[k]] = name

        self.cps_names = self.context['output']['language']['cps'].names

    def find_cb_nodes(self):
        self.cb_nodes = []
        for i in self.model.all_node_map:
            if self.model.all_node_map[i].tag in self.supported_list_containing_cb:
                self.cb_nodes.append(i)

        self.cb_node_keys = {}

        for k in self.cb_nodes:
            if k not in self.model.container_keys:
                print "Missing " + k
                print self.model.container_keys
                raise Exception("Missing key")
            else:
                keys_raw = self.model.container_keys[k]

            keys = ""
            keys_raw = keys_raw.split()
            if self.all_node_map[k].get('augmented') is True:
                keys_raw = keys_raw[2:]
            else:
                keys_raw = keys_raw[1:]
            for str in keys_raw:
                keys += self.names[str] + ","
            keys = keys[:-1]

            self.cb_node_keys[self.names[k]] = keys

        self.cb_node_keys_data = {}
        for k in self.cb_nodes:
            if k not in self.model.container_keys:
                print "Missing " + k
                print self.model.container_keys
                raise Exception("Missing key")
            keys_raw = self.model.container_keys[k]

            keys = ""
            keys_raw = keys_raw.split()
            if self.all_node_map[k].get('augmented') is True:
                keys_raw = keys_raw[2:]
            else:
                keys_raw = keys_raw[1:]
            for str in keys_raw:
                keys += self.names[str] + " "

            self.cb_node_keys_data[self.names[k]] = keys

    def setup(self, model):
        self.model = model
        self.prefix = self.model.module.prefix
        self.module = self.model.module.module
        self.module_obj = self.model.module

        __combined_map = {}
        for mod in self.model.context['loader'].yin_map.keys():
            __all_node_map = self.model.context['loader'].yin_map[mod].all_node_map
            __combined_map.update(__all_node_map)
        self.all_node_map = __combined_map

        self.supported_list_containing_cb = self.model.module.prepend_ns_to_list(
            self.supported_list_containing_cb)

        self.setup_names()
        self.find_cb_nodes()


    def rw_access(self, yin_node):
        config_access = yin_node.find(self.model.module.ns() + 'config')
        if config_access is not None:
            access = config_access.find('value') == 'true'
            return access
        return True

    def is_parent_tree_read_only(self, cb_node):
        real_path = self.names[cb_node]
        model = self.model
        while real_path.find('/') != -1:
            node = self.all_node_map[real_path]
            if "augment" in node.tag:
                 model = node.get('model')
                 #splice augmenting prefix from real_path
                 slash_index = real_path.find('/')
                 real_path = real_path[slash_index+1:]
                 node = self.all_node_map[real_path]
            if self.rw_access(node) == False:
                return True
            real_path = model.parent[real_path]
        return False

    def get_leaves(self, full_name):
        leaves = []
        for leaf in self.model.container_map[full_name]:
            leaf = leaf.name
            yin_node = self.all_node_map[leaf]
            if yin_node.tag != self.model.module.ns() + 'leaf' and yin_node.tag != self.model.module.ns() + 'leaf-list':
                continue
            leaves.append(leaf)
        return leaves

    def get_node_leaves_based_on_access(self, cb_node, read_only):
        full_name = self.names[cb_node]
        parent_ro = self.is_parent_tree_read_only(cb_node)

        ro_l = []
        rw_l = []
        yin_node = self.all_node_map[full_name]
        is_rpc = False
        if yin_node.tag == self.model.module.ns() + 'rpc':
            is_rpc = True
            if read_only:
                full_name = full_name + '/output'
                parent_ro = True
            else:
                full_name = full_name + '/input'
            if full_name not in self.model.container_map:
                return rw_l
        for leaf in self.model.container_map[full_name]:
            leaf = leaf.name
            yin_node = self.all_node_map[leaf]
            if yin_node.tag != self.model.module.ns() + 'leaf' and yin_node.tag != self.model.module.ns() + 'leaf-list':
                if is_rpc:
                    if yin_node.tag == self.model.module.ns() + 'container':
                        full_name = leaf
                        leaves = self.get_leaves(full_name)
                        for leaf in leaves:
                            if read_only:
                                ro_l.append(leaf)
                            else:
                                rw_l.append(leaf)
                    if yin_node.tag == self.model.module.ns() + 'choice':
                        full_name = leaf
                        for leaf in self.model.container_map[full_name]:
                            leaf = leaf.name
                            full_name = leaf
                            leaves = self.get_leaves(full_name)
                            for leaf in leaves:
                                if read_only:
                                    ro_l.append(leaf)
                                else:
                                    rw_l.append(leaf)
                    continue
                else:
                    continue
            if parent_ro or self.rw_access(yin_node) == False:
                ro_l.append(leaf)
            else:
                rw_l.append(leaf)
        if read_only:
            return ro_l
        return rw_l

    def node_rw_access(self, cb_node):
        full_name = self.names[cb_node]
        yin_node = self.all_node_map[full_name]
        if self.rw_access(yin_node) == False:
            return False
        l = self.get_node_leaves_based_on_access(cb_node, False)
        if len(l) == 0:
            return False
        return True

    def get_keys(self, cb_node, object):
        keys = self.cb_node_keys_data[cb_node]
        line = ""
        print "  /* Keys start... */ "
        for i in keys.split():
            full_name = self.names[i]
            if full_name in self.all_node_map.keys():
                model = self.model
                yin_node = self.all_node_map[full_name]
            else:
                for model in self.model.augment_list:
                    if full_name in self.all_node_map.keys():
                        yin_node = self.all_node_map[full_name]
            if yin_node is not None:
                if yin_node.tag == model.module.ns() + 'leaf' or yin_node.tag == model.module.ns() + 'leaf-list':
                    print "  cma_value_t " + self.names_short[i] + "_kval;"
                    if self.names[i] not in self.aug_names:
                        key_name = i
                    else:
                        key_name = self.aug_names[self.names[i]]
                    print "  const bool " + self.names_short[i] + "_kval_valid = cma_get_key_data(" + object + "," + key_name + ",&" + self.names_short[i] + "_kval);"
                    print "  (void)" + self.names_short[i] + "_kval_valid;"
                    print ""

        print "  /* Keys end... */ "

    def get_instance_vars(self, cb_node, read_only, function):
        full_name = self.names[cb_node]
        print "  /* Instance vars start... */ "

        if function.find('get') != -1:
            print "  cps_api_object_it_begin(obj,&it);"
            print "  while (cps_api_object_it_valid(&it)) {"
            print "      cma_value_t val;"
            print "      cps_api_attr_id_t id = cps_api_object_attr_id(it.attr);"
            print "      cps_api_object_attr_t attr = cps_api_object_attr_get(obj,id);"
            print "      if (attr==NULL) {"
            print "          cps_api_object_it_next(&it);"
            print "          continue;"
            print "      }"
            print ""
            print "      /* Extract data from CPS obj and add it to your structure */"
            print "      switch(id) {"
        leaf_present=False
        for leaf in self.get_node_leaves_based_on_access(cb_node, read_only):
            if self.all_node_map[leaf].tag == self.model.module.ns() + 'leaf-list':
                if function.find('get') != -1:
                    leaf_present = True
                    print "          case " + self.aug_names[leaf] + ":"
                    print "              /* Iterate inside for the leaf-list */"
                    print "              for(cma_get_tag_it_inside(obj," + self.aug_names[leaf] + ",&it);"
                    print "                  cps_api_object_it_valid(&it);"
                    print "                  cps_api_object_it_next(&it)) {"
                    print "                  if(cma_get_data_fr_it(&it,&val)){;"
                    print "                      /* process data here - i.e. val_valid */ "
                    print "                      ;"
                    print "                  }"
                    print "              }"
                    print "              break;"
                else:
                    print "  /*Update this field with the correct data before setting into object*/"
                    print "  cma_value_t " + self.names_short[self.names[leaf]] + "_val;"
                    print "  /*Repeat for least-list for as many entries available*/"
                    print ""
            else:
                if function.find('get') != -1:
                    leaf_present = True
                    print "          case " + self.aug_names[leaf] + ":"
                    print "              if(cma_get_data_fr_it(&it,&val)){"
                    print "                  val_is_null(val)? "
                    print "                     EV_LOG_INFO(MGMT_LOG_SUBSYSTEM,0,\"CMA\",\", value of " + self.names_short[self.names[leaf]] + " is to be deleted.\"):"
                    print "                     cma_dump_value_with_name(&val,\"" + self.names_short[self.names[leaf]] + "\");"
                    print "              }"
                    print "              break;"
                else:
                    print "  cma_value_t " + self.names_short[self.names[leaf]] + "_val;"
                    print "  const bool " + self.names_short[self.names[leaf]] + "_val_valid = " + function + "(obj," + self.aug_names[leaf] + ",&" + self.names_short[self.names[leaf]] + "_val);"
                    print "  (void)" + self.names_short[self.names[leaf]] + "_val_valid;"
                    print ""
        if function.find('get') != -1  and leaf_present==True:
            print "          default:"
            print "             break;"
            print "      }"
            print "      cps_api_object_it_next(&it);"
            print "  }"
        print ""

        print "  /* Instance vars end... */ "

    def spit_rpc_node(self, cb_node):
        print "cps_api_return_code_t _rpc_" + self.get_aug_key_for_key(cb_node) + "(void * context, cps_api_transaction_params_t * param, size_t key_ix) {"
        print ""
        print "  EV_LOG_INFO(MGMT_LOG_SUBSYSTEM,0,\"CMA\","
        print "     \"_rpc_" + self.get_aug_key_for_key(cb_node) + " called.\\n\"); "
        print "  /*iterator for leaf-list*/"
        print "  cps_api_object_it_t it;"
        print "  (void)it;"
        print ""
        print "  /*object that contains the data to set*/ "
        print "  cps_api_object_t obj = cps_api_object_list_get(param->change_list,key_ix);"
        print ""
        print ""
        self.get_instance_vars(cb_node, False, "cma_get_data")
        self.get_instance_vars(cb_node, True, "cma_set_data")
        print ""
        print "  /*Return a cps_api_ret_code_OK when you implement and have a successful operation*/"
        print "  return cps_api_ret_code_OK;"
        print "}"
        print ""

    def read_cb_node(self, cb_node):
        print "static cps_api_return_code_t _get_" + self.get_aug_key_for_key(cb_node) + " (void * context, cps_api_get_params_t * param, size_t key_ix) {"
        print ""
        print "  EV_LOG_INFO(MGMT_LOG_SUBSYSTEM,0,\"CMA\","
        print "     \"_get_" + self.get_aug_key_for_key(cb_node) + " called.\\n\"); "
        print "  cps_api_object_t filter = cps_api_object_list_get(param->filters,key_ix);"
        print "  cps_api_key_t *key    = cps_api_object_key(filter);"
        print "  cps_api_object_t obj = cps_api_object_create();"
        print ""
        print "  if (obj==NULL) return cps_api_ret_code_ERR;"
        print "  if (!cps_api_object_list_append(param->list,obj)) {"
        print "     cps_api_object_delete(obj);"
        print "     return cps_api_ret_code_ERR;"
        print "  }"
        print ""
        print "  cps_api_key_copy(cps_api_object_key(obj),key);"
        self.get_keys(cb_node, "filter")
        print ""
        self.get_instance_vars(cb_node, True, "cma_set_data")
        print ""
        print "  /*Return a cps_api_ret_code_OK when you implement and have a successful operation*/"
        print "  return cps_api_ret_code_OK;"
        print "}"
        print ""

    def write_cb_node(self, cb_node):
        print "cps_api_return_code_t _set_" + self.get_aug_key_for_key(cb_node) + "(void * context, cps_api_transaction_params_t * param, size_t key_ix) {"
        print ""
        print "  /*iterator for leaf-list*/"
        print "  cps_api_object_it_t it;"
        print "  (void)it;"

        print ""
        print "  /*object that contains the data to set*/ "
        print "  cps_api_object_t obj = cps_api_object_list_get(param->change_list,key_ix);"
        print "  cps_api_object_t prev = cps_api_object_list_get(param->prev,key_ix); (void)prev;"
        print "  cma_edit_mode_t edit_mode;"
        print "  cma_edit_mode(obj,&edit_mode);"

        print "  EV_LOG_INFO(MGMT_LOG_SUBSYSTEM,0,\"CMA\","
        print "     \"_set_" + self.get_aug_key_for_key(cb_node) + " called. phase is %d and op is %d.\\n\", "
        print "     edit_mode.phase, edit_mode.op);"

        print ""
        self.get_keys(cb_node, "obj")
        print ""

        self.get_instance_vars(cb_node, False, "cma_get_data")

        print""
        print write_statement_switch
        print ""
        print "  (void)retval;"
        print "  /*Return a cps_api_ret_code_OK when you implement and have a successful operation"
        print "   also can return a module specific return code.*/"
        print "  return cps_api_ret_code_OK;"
        print "}"
        print ""

    def write_main_h(self):
        module_us = self.name_to_cms_name(self.module)
        print "#ifndef _" + module_us + "_init_h_"
        print "#define _" + module_us + "_init_h_"
        print "void " +  module_us + "_init();"
        print "#endif"

    def write_main_c(self):
        module_us = self.name_to_cms_name(self.module)
        for elem in self.cb_node_keys:
            if "augment" not in self.all_node_map[self.names[elem]].tag:
                print "#include  \""+ self.get_aug_key_for_key(elem) + ".h\""
        print "#include  \"" + self.module + ".h\""
        print "#include  \"" + module_us + "_init.h\""
        print "#include  \"cma_init.h\""
        print "#include  \"cma_utilities.h\""
        print "#include  \"std_utils.h\""
        print ""
        print ""
        if self.module_obj.get_if_augments() != True:
            print "static cma_cms_reg_t modreg[] = {"
            print "  { .module_name =" +  module_us.upper() +  "_MODEL_STR,.module_revision = \"\", .has_default_db = false }"
            print "};"
            print ""
        print ""
        print "void " +  module_us + "_init()"
        print ""
        print "{"
        print ""
        for elem in self.cb_node_keys:
            if "augment" not in self.all_node_map[self.names[elem]].tag:
                print "    " +  "cma_init_" + self.get_aug_key_for_key(elem) +"();"
        #  If the model only augments, there is no need to register
        if self.module_obj.get_if_augments() != True:
            print "    /* Register with CMS to get the startup config replayed */"
            print "    if (cma_module_register(1, modreg) !=true) {"
            print "        EV_LOG(ERR,MGMT,0,\"CMA\",\"Error registering modules has failed\\n\");"
            print "        return;"
            print "    }"
            print ""
            print "    EV_LOG_INFO(MGMT_LOG_SUBSYSTEM,0,\"CMA\",\"Interface cma registered to CMS SUCCESS.\\n\");"
        print ""
        print "}"

    def get_aug_key_for_key(self, key):
        name = self.names[key]
        if name in self.aug_names.keys():
            aug_key = self.aug_names[name]
            return aug_key
        else:
           return key

    def write_init(self, elem, read_res, write_res, rpc_res):
        print "void cma_init_"  + self.get_aug_key_for_key(elem) + "(void) {"
        print "  cps_api_registration_functions_t f;"
        keys_list = self.cb_node_keys[elem].split(',')
        two_keys = keys_list[:2]
        two_keys_aug = []
        #print "0 is " + two_keys[0] + " 1 is " + two_keys[1]
        two_keys_aug.append(self.get_aug_key_for_key(two_keys[0]))
        if len(two_keys) < 2:
            two_keys_aug.append("0")
        else:
            two_keys_aug.append(self.get_aug_key_for_key(two_keys[1]))
        rest_keys = keys_list[2:]

        print "  memset(&f,0,sizeof(f));"
        print "  "
        line = ""
        line += "cps_api_key_init(&f.key,cps_api_qualifier_TARGET," + ','.join(
            two_keys_aug) + ","
        line += str(len(rest_keys))
        for rk in rest_keys:
            line += "," + self.get_aug_key_for_key(rk)
        line += ");"
        print "  " + line
        if read_res:
            print "  f._read_function=_get_" + self.get_aug_key_for_key(elem) + ";"
        else:
            print "  f._read_function=NULL;"
        if write_res:
            print "  f._write_function=_set_" + self.get_aug_key_for_key(elem) + ";"
        elif rpc_res:
            print "  f._write_function = _rpc_" + self.get_aug_key_for_key(elem) + ";"
        else:
            print "  f._write_function=NULL;"
        print "  f._rollback_function=NULL;"
        print "  cma_api_init(&f,1);"
        print"}"

    def write_headers(self, elem):
        print "/*Generated for " + self.get_aug_key_for_key(elem) + "*/"
        print cma_gen_file_c_includes
        print "#include \"" + self.module + "_xmltag.h\""
        node = self.model.all_node_map[self.names[elem]]
        if node.get('augmented') == True:
            for aug_mod in self.model.augment_list:
                print "#include \"" + aug_mod.module.module + "_xmltag.h\""

    def xmltag_mapping(self):
        print "/* OPENSOURCELICENSE */"
        print "#ifndef __" + self.name_to_cms_name(self.module) + "_xmltag_cma_h"
        print "#define __" + self.name_to_cms_name(self.module) + "_xmltag_cma_h"
        print "#include \"" + self.module + ".h\""
        print "/*Autogenerated file from " + self.module + "*/"
        print ""
        for i in self.names:
            if i in self.model.all_node_map and self.model.is_id_element(self.model.all_node_map[i]):
                print "#define " + self.aug_names[i] + " " + self.cps_names[i] + " "

        print "#endif"

    def xmltag_init_mapping_hdr(self):
        print "/* OPENSOURCELICENSE */"
        print "#ifndef __" + self.name_to_cms_name(self.module) + "_init_mapping_xmltag_h"
        print "#define __" + self.name_to_cms_name(self.module) + "_init_mapping_xmltag_h"

        print """

#ifdef __cplusplus
#include <unordered_map>
#include <string>
#include <stdint.h>

void init_""" + self.name_to_cms_name(self.module) + """_xmltag(std::unordered_map<std::string,uint32_t> &ref);

#endif
#endif
"""

    def xmltag_mapping_src(self):
        print "/* OPENSOURCELICENSE */"
        print "#include \"" + self.module + ".h\""
        print "#include \"" + self.module + "_xmltag.h\""
        print ""
        print "/*Autogenerated file from " + self.module + "*/"
        str = """
#include <string>
#include <unordered_map>

using un_name_to_id = std::unordered_map<std::string,uint32_t>;
static un_name_to_id _map = {
            """
        print str
        for i in self.names:
            if i in self.model.all_node_map and self.model.is_id_element(self.model.all_node_map[i]):
                print "{\"" + self.aug_names[i] + "\"," + self.cps_names[i] + "},"

        print "};"
        print ""
        print """
void init_""" + self.name_to_cms_name(self.module) + """_xmltag(std::unordered_map<std::string,uint32_t> &ref) {
    ref.insert(_map.begin(),_map.end());
}

"""

    def write(self):
        old_stdout = sys.stdout

        with open(os.path.join(self.context['args']['cmssrc'], self.name_to_cms_name(self.module) + "_init.h"), "w") as sys.stdout:
            self.write_main_h()

        with open(os.path.join(self.context['args']['cmssrc'], self.name_to_cms_name(self.module) + "_init.c"), "w") as sys.stdout:
            self.write_main_c()

        with open(os.path.join(self.context['args']['cmsheader'], self.module + "_xmltag.h"), "w") as sys.stdout:
            self.xmltag_mapping()

        with open(os.path.join(self.context['args']['cmsheader'], self.module + "_init_mapping_xmltag.h"), "w") as sys.stdout:
            self.xmltag_init_mapping_hdr()

        with open(os.path.join(self.context['args']['cmssrc'], self.module + "_xmltag.cpp"), "w") as sys.stdout:
            self.xmltag_mapping_src()

        for elem in self.cb_node_keys:

            elem_name = self.names[elem]
            if elem_name in self.model.all_node_map.keys():
                # No need to register the augment node itself.
                if "augment" not in self.model.all_node_map[self.names[elem]].tag:
                    with open(os.path.join(self.context['args']['cmssrc'], self.get_aug_key_for_key(elem) + ".h"), "w") as sys.stdout:
                        print "/* OPENSOURCELICENSE */"
                        print "#ifndef __" + self.get_aug_key_for_key(elem) + "_cma_h"
                        print "#define __" + self.get_aug_key_for_key(elem) + "_cma_h"
                        print "#ifdef __cplusplus"
                        print "extern \"C\" {"
                        print "#endif"
                        print "/*Autogenerated file from " + self.module + "*/"
                        print "void cma_init_" + self.get_aug_key_for_key(elem) + "(void) ;"
                        print ""
                        print "#ifdef __cplusplus"
                        print "}"
                        print "#endif"
                        print ""
                        print "#endif"

        for elem in self.cb_node_keys:
            with open(os.path.join(self.context['args']['cmssrc'], self.get_aug_key_for_key(elem) + ".c"), "w") as sys.stdout:
                self.write_headers(elem)
                full_name = self.names[elem]
                yin_node = self.model.all_node_map[full_name]
                read_res = False
                write_res = False
                if yin_node.tag == self.model.module.ns() + 'rpc':
                    rpc_res = True
                    self.spit_rpc_node(elem)
                    if "augment" not in self.model.all_node_map[self.names[elem]].tag:
                        self.write_init(elem, read_res, write_res, rpc_res)
                    print ""
                else:
                    rpc_res = False
                    read_res = False
                    ro_node = self.node_rw_access(elem) == False
                    rw_elems = len(
                        self.get_node_leaves_based_on_access(elem, False)) != 0
                    ro_elems = len(
                        self.get_node_leaves_based_on_access(elem, True)) != 0

                    if ro_node or ((not ro_node) and ro_elems):
                        self.read_cb_node(elem)
                        read_res = True

                    write_res = False

                    if (not ro_node) and rw_elems:
                        self.write_cb_node(elem)
                        write_res = True


                    if "augment" not in self.model.all_node_map[self.names[elem]].tag:
                        self.write_init(elem, read_res, write_res, rpc_res)
                    print ""
        sys.stdout = old_stdout

    def close(self):
        print ""
