import cps
import os

import bytearray_utils

class CPSTypes:
    def __init__(self):
        self.types = {}

    def add_type(self, key, typ):
        self.types[key] = typ

    def guess_type_for_len(self,val):
        if type(val) == int:
            return 'uint32_t'
        if type(val) == bytearray:
            if len(val)==4:
                return 'uint32_t'
        return None

    def find_type(self,key,val):
        if key in self.types:
            t = self.types[key]
        else:
            t = self.guess_type_for_len(val)
            if t==None:
                t = 'bytearray'
        return t

    def from_data(self, key, val):
        t = self.find_type(key,val)
        return bytearray_utils.ba_to_value(t,val)

    def to_data(self,key,val):
        t = self.find_type(key,val)
        return bytearray_utils.value_to_ba(t,val)

    def print_object(self,obj):
        data = obj['data']
        print "Key: "+obj['key']
        for k in data:
            print k +" = "+str(self.from_data(k,data[k]))

class CPSLibInit:
    def load_class_details(self):
        libs=[]
        path=os.getenv("LD_LIBRARY_PATH")
        if path==None:
            path='/opt/ngos/lib'
        for i in path.split(':'):
            print "Searching "+i
            files = os.listdir(i)
            for f in files:
                if f.find('cpsclass')==-1:continue
                libs.append(i)
                break;
        for i in libs:
            print "Loading from "+i
            res = cps.init(i,'cpsclass')
            print res

    def __init__(self):
        self.load_class_details()


def init():
    CPSLibInit()

def key_mapper():
    return CPSKeys()

cps_attr_types_map = CPSTypes()

def cps_add_attr_type(attr_str,val):
    cps_attr_types_map.add_type(attr_str,val)

def create_cps_transaction_object(op,qual,module):
    """
    Create a cps object for performing transaction
    @op = operation type ("create","delete","set")
    @qual = qualifier type ("target","observed",..)
    @module = module key string ("base-xxx/yyy")
    @return a cps object
    """
    cps_op = {}
    obj = {'key':'','data':{}}
    cps_op['change'] = obj
    cps_op['operation'] = op
    cps_op['root_path'] = module+"/"
    obj['key'] = cps.key_from_name(qual,module)
    return cps_op

def cps_generate_attr_path(cps_object, attr_str):
    if "/" in attr_str:
        return attr_str
    else:
        return cps_object['root_path']+attr_str

def cps_object_add_attr(cps_object,attr_str,val):
    """
    Add Attributes to cps object
    @cps_object = cps object
    @attr_str = attr string in yang("id", "base-port/interface/id")
    @val = value of attribute
    @return none
    """
    cps_object['change']['data'][cps_generate_attr_path(cps_object,attr_str)] = \
    cps_attr_types_map.to_data(cps_generate_attr_path(cps_object,attr_str),val)

def create_cps_get_object(qual,module):
    """
    Create a cps get operation object
    @qual = qualifier type ("target","observed",..)
    @module = module string ("base-xxx/yyy")
    @return a cps get object with keys populated
    """
    obj = {'key':'','data':{}}
    obj['key'] = cps.key_from_name(qual,module)
    obj['root_path'] = module+"/"
    return obj

def cps_object_add_filter(cps_get_object,types_obj,attr_str,val):
    """
    Add Filter Attributes to cps_get_object
    @cps_get_object = cps get object
    @attr_str = attr string in yang("id", "port")
    @val = value of attribute
    @return a dictioanry with all cps data populated
    """
    cps_get_object['data'][cps_generate_attr_path(cps_get_object,attr_str)] = \
    cps_attr_types_map.to_data(cps_generate_attr_path(cps_get_object,attr_str),val)



