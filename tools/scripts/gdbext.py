# This is a Python script for GDB to help debug OpenDDS.
#
# Features:
#   - Properly display GUID/BuiltinTopicKeys, GUID Prefixes, and GUID Entity
#     IDs with known built-in names
#   - Shortcuts to important global singletons
#   - Shortcuts to all participants, discoveries, SPDP and SEDP instances, user
#     readers, and user writers
#   - Helpers to make it easier to work with data structures in Python:
#     value_is, std_*_values, deref, deref_holder.
#
# To use:
#   - Run `gdb -x path/to/gdbext.py ...` for a new session
#   - `source path/to/gdbext.py` for an existing session
#   - You might run into this python exception comming from libstdcpp.py:
#       'NoneType' object has no attribute 'pointer'
#     This is a GDB patch that might fix it:
#       https://sourceware.org/pipermail/libstdc++/2020-December/051773.html
#     However it can also be fixed by switching to a frame of a C++ function
#     and sourcing the file again.
#
# Tested with Ubuntu 22.04 (GDB 12.1, gcc 11.4, and libstdc++ 3.4.30)
# This script might break with changes to either OpenDDS, GDB, or gcc/libstdc++
#
# Ideas:
#   - Printers for:
#     - Time types (*TimePoint, TimeDuration, Time_t, MonotonicTime_t, and
#       ACE_Time_Value)
#   - GDB commands for getting values from standard containers using the
#     std_*_values functions

from itertools import islice
from textwrap import wrap
import socket

import gdb
import libstdcxx


line_len = 100

def header(line, fill):
    print(line, fill * (line_len - len(line) - 1))

header('OpenDDS GDB Extension', '=')


def batched(iterable, n, t=tuple):
    if n < 1:
        raise ValueError('n must be at least one')
    iterator = iter(iterable)
    while True:
        batch = t(islice(iterator, n))
        if not batch:
            break
        yield batch


def value_is(value, *type_names):
    resolved = value.type.strip_typedefs()
    names = (value.type.name, resolved.name, value.type.tag, resolved.tag)
    for type_name in type_names:
        if type_name in names:
            return True
    return False


def get_byte_slice(value, size, as_type):
    return as_type([int(value[i]) for i in range(0, size)])

def get_byte_array(value):
    return get_byte_slice(value, value.type.strip_typedefs().range()[1] + 1, tuple)

def std_string_value(value, enc='utf-8'):
    p = value['_M_dataplus']['_M_p']
    t = value.type.strip_typedefs()
    if t.name.find("::__cxx11::basic_string") != -1:
        l = value['_M_string_length']
        p = p.cast(p.type.strip_typedefs())
    else:
        if t.code == gdb.TYPE_CODE_REF:
            t = t.target()
        real = t.unqualified().strip_typedefs()
        rep = gdb.lookup_type(str(real) + '::_Rep').pointer()
        l = (ptr.cast(rep) - 1).dereference()['_M_length']
    string = get_byte_slice(p, l, bytes)
    if enc is not None:
        string = string.decode(enc)
    return string


def _std_values(StdPrinter, value):
    printer = StdPrinter('', value)
    try:
        values = printer.children()
    except AttributeError as ex:
        print(
            'The following exception is a bug in libstdcxx. This is a patch that might fix',
            'this:',
            '  https://sourceware.org/pipermail/libstdc++/2020-December/051773.html',
            'However it can also be fixed by switching to a frame of a C++ function',
            'and trying again.',
            sep='\n', file=sys.stderr)
        raise ex
    for e in values:
        yield e[1]

def std_vector_values(value):
    return _std_values(libstdcxx.v6.printers.StdVectorPrinter, value)

def std_set_values(value):
    return _std_values(libstdcxx.v6.printers.StdSetPrinter, value)

def _std_pairs(StdPrinter, value):
    return batched(_std_values(StdPrinter, value), 2)

def std_map_values(value):
    return _std_pairs(libstdcxx.v6.printers.StdMapPrinter, value)

def std_unordered_map_values(value):
    return _std_pairs(libstdcxx.v6.printers.Tr1UnorderedMapPrinter, value)


def deref(ptr):
    if int(ptr) == 0:
        return None
    # I'd think this should be a dynamic_cast instead of cast, but dynamic_cast
    # doesn't seem to work.
    return ptr.cast(ptr.dynamic_type).dereference()


def deref_holder(value):
    return deref(value['ptr_'])


def get_singleton(name):
    singleton_obj = deref(gdb.parse_and_eval(name + '::singleton_'))
    return None if singleton_obj is None else singleton_obj['instance_']


def printer_for_types(printer, *type_names):
    gdb.printing.register_pretty_printer(gdb.current_objfile(),
        lambda v: printer(v) if value_is(v, *type_names) else None)


def first_field(value):
    return value[value.type.fields()[0].name]


class GuidPrinterBase:
    def __init__(self, array):
        self.array = array

    def unknown(self):
        return not any(self.array)

    def _array_as_hex_str(self, sep='.'):
        return sep.join(batched(['{:02x}'.format(b) for b in self.array], 4, ''.join))

    def _to_string(self, hint):
        s = self._array_as_hex_str()
        return '{} ({})'.format(s, hint) if hint else s

    def __str__(self):
        return self.to_string()

    def __repr__(self):
        return str(self)


class GuidPrefixPrinter(GuidPrinterBase):

    def __init__(self, value):
        GuidPrinterBase.__init__(self, get_byte_array(value))

    def to_string(self):
        return self._to_string('unknown prefix' if self.unknown() else None)

printer_for_types(GuidPrefixPrinter, 'OpenDDS::DCPS::GuidPrefix_t')


class GuidEntityPrinter(GuidPrinterBase):

    def __init__(self, value=None, entity_key=None, entity_kind=None):
        if value is not None:
            self.entity_key = get_byte_array(value['entityKey'])
            self.entity_kind = int(value['entityKind'])
        else:
            self.entity_key = entity_key
            self.entity_kind = entity_kind
        GuidPrinterBase.__init__(self, self.entity_key + (self.entity_kind,))

    builtins = {
        (0x00, 0x00, 0x02): 'SEDP topic',
        (0x00, 0x00, 0x03): 'SEDP publications',
        (0xff, 0x00, 0x03): 'secure SEDP publications',
        (0x00, 0x00, 0x04): 'SEDP subscriptions',
        (0xff, 0x00, 0x04): 'secure SEDP subscriptions',
        (0x00, 0x01, 0x00): 'SPDP participant',
        (0xff, 0x01, 0x01): 'secure SPDP reliable participant',
        (0x00, 0x02, 0x00): 'P2P participant message',
        (0xff, 0x02, 0x00): 'secure P2P participant message',
        (0x00, 0x02, 0x01): 'P2P participant stateless',
        (0xff, 0x02, 0x02): 'secure P2P participant volatile message',
        (0x00, 0x03, 0x00): 'type lookup request',
        (0xff, 0x03, 0x00): 'secure type lookup request',
        (0x00, 0x03, 0x01): 'type lookup reply',
        (0xff, 0x03, 0x01): 'secure type lookup reply',
    }

    kinds = {
        0xc0: 'builtin unknown',
        0xc1: 'participant',
        0xc2: 'writer', # (builtin keyed writer)
        0xc3: 'writer', # (builtin unkeyed writer)
        0xc4: 'reader', # (builtin unkeyed reader)
        0xc7: 'reader', # (builtin keyed reader)
        0xc5: 'builtin topic',
        0x00: 'user unknown',
        0x02: 'keyed writer',
        0x03: 'unkeyed writer',
        0x04: 'unkeyed reader',
        0x07: 'keyed reader',
        0x41: 'OpenDDS subscriber',
        0x42: 'OpenDDS publisher',
        0x45: 'OpenDDS topic',
        0x4a: 'OpenDDS user',
    }

    def builtin(self):
        return self.entity_kind >> 4 == 0xc

    def hint(self):
        if self.unknown():
            return 'unknown entity'
        hint = self.kinds.get(self.entity_kind, 'unrecognized kind')
        if self.builtin() and self.entity_kind != 0xc1:
            hint = self.builtins.get(self.entity_key, 'unrecognized builtin') + ' ' + hint
        return hint

    def to_string(self):
        return self._to_string(self.hint())

printer_for_types(GuidEntityPrinter, 'OpenDDS::DCPS::EntityId_t')


class GuidPrinter(GuidPrinterBase):

    def __init__(self, value):
        if value_is(value, 'OpenDDS::DCPS::GUID_t'):
            self.entity = GuidEntityPrinter(value['entityId'])
            array = GuidPrefixPrinter(value['guidPrefix']).array + self.entity.array
        elif value_is(value, 'DDS::BuiltinTopicKey_t'):
            array = self.get_byte_array(value['value'])
            self.entity = GuidEntityPrinter(entity_key=array[12:16], entity_kind=array[15])
        else:
            raise TypeError('Not sure what type is')
        GuidPrinterBase.__init__(self, array)

    def to_string(self):
        return self._to_string('unknown guid' if self.unknown() else self.entity.hint())

    def to_plain_string(self):
        return self._to_string(None)

    def to_id_string(self):
        return self._array_as_hex_str(sep='_')

printer_for_types(GuidPrinter, 'OpenDDS::DCPS::GUID_t', 'DDS::BuiltinTopicKey_t')


class AddressPrinter:

    def __init__(self, value):
        self.family = None
        self.host = None
        self.port = 0
        if value_is(value, 'OpenDDS::DCPS::NetworkAddress', 'ACE_INET_Addr'):
            base = value['inet_addr_']['in4_']
            self._set_family(base['sin_family'])
            if self.is_ipv4():
                value = base
            elif self.is_ipv6():
                value = value['inet_addr_']['in6_']

        if value_is(value, 'sockaddr_in'):
            self._set_family(value['sin_family'])
            if self.is_ipv4():
                self._set_port(value['sin_port'])
                value = value['sin_addr']
        elif value_is(value, 'sockaddr_in6'):
            self._set_family(value['sin6_family'])
            if self.is_ipv6():
                self._set_port(value['sin6_port'])
                value = value['sin6_addr']

        if value_is(value, 'in_addr'):
            self._set_family(socket.AF_INET)
            self.host = list(socket.ntohl(int(value['s_addr'])).to_bytes(4, 'big'))
        elif value_is(value, 'in6_addr'):
            self._set_family(socket.AF_INET6)
            try:
                addr_value = value['s6_addr']
            except gdb.error:
                # glibc uses a union, try this
                addr_value = first_field(first_field(value))
            self.host = get_byte_array(addr_value)

    def _set_family(self, value):
        if self.family is None:
            self.family = int(value)

    def is_ipv4(self):
        return self.family == socket.AF_INET

    def is_ipv6(self):
        return self.family == socket.AF_INET6

    def _set_port(self, value):
        self.port = socket.ntohs(int(value))

    def host_str(self):
        if self.is_ipv4():
            return '.'.join([str(b) for b in self.host])
        elif self.is_ipv6():
            pair_to_hex = lambda pair: '{:04x}'.format(int.from_bytes(pair, byteorder='big'))
            return '[' + ':'.join([pair_to_hex(p) for p in batched(self.host, 2)]) + ']'
        else:
            return '(unknown address family {})'.format(self.family)

    def to_string(self):
        s = self.host_str()
        if self.port:
            s += ':{}'.format(self.port)
        return s

printer_for_types(AddressPrinter,
    'OpenDDS::DCPS::NetworkAddress',
    'ACE_INET_Addr',
    'in_addr',
    'sockaddr_in',
    'in6_addr',
    'sockaddr_in6',
)


def set_a_var(name, desc, value, indent=0):
    i = lambda l: '  ' * l
    note = ' (invalid or uninitialized)' if value is None else ''
    print(*wrap('- ${} is {}{}'.format(name, desc, note), width=line_len,
        initial_indent=i(indent), subsequent_indent=i(indent + 1)), sep='\n')
    # Set it both in GDB and Python
    gdb.set_convenience_variable(name, value)
    globals()[name] = value


set_a_var('service_part', 'the service participant',
    get_singleton('ACE_Singleton<OpenDDS::DCPS::Service_Participant,ACE_Thread_Mutex>'))

set_a_var('part_factory', 'the participant factory',
    None if service_part is None else deref_holder(service_part['dp_factory_servant_']))

set_a_var('transport_reg', 'the transport registry',
    get_singleton('ACE_Unmanaged_Singleton<OpenDDS::DCPS::TransportRegistry,ACE_Recursive_Thread_Mutex>'))


def get_discoveries():
    header('Discoveries', '-')
    for name, disc_rc in std_map_values(service_part['discoveryMap_']):
        disc = deref_holder(disc_rc)
        set_a_var('disc_' + std_string_value(name), 'a ' + disc.type.name, disc)

        if disc.type.name == 'OpenDDS::RTPS::RtpsDiscovery':
            for domain, part_map in std_map_values(disc['participants_']):
                for part_guid, part_handle in std_map_values(part_map):
                    spdp = deref_holder(part_handle)
                    set_a_var('spdp_' + GuidPrinter(part_guid).to_id_string(),
                        'a ' + spdp.type.tag, spdp, 1)
                    sedp = deref_holder(spdp['sedp_'])
                    set_a_var('sedp_' + GuidPrinter(part_guid).to_id_string(),
                        'a ' + sedp.type.tag, sedp, 1)

if service_part:
    get_discoveries()


def get_transports():
    header('Transports', '-')
    for name, ti_rc in std_map_values(transport_reg['inst_map_']):
        ti = deref_holder(ti_rc)
        set_a_var('ti_' + std_string_value(name), 'a ' + ti.type.name, ti)

if transport_reg:
    get_transports()


def get_entities():
    all_part = {}
    all_dw = {}
    all_dr = {}
    for domain, parts in std_map_values(part_factory['participants_']):
        header('domain ' + str(int(str(domain), base=0)), '-')
        part_count = 0
        for part_ptr in std_set_values(parts):
            part = deref_holder(part_ptr)
            gp = GuidPrinter(part['dp_id_'])
            all_part[gp.to_plain_string()] = part
            part_var_id = gp.to_id_string()
            part_var = 'part_' + part_var_id
            set_a_var(part_var, 'a participant', part)

            pub_count = 0
            for pub_ptr in std_set_values(part['publishers_']):
                pub = pub_ptr['svt_'].dereference()
                set_a_var('pub_{}_{}'.format(part_var_id, pub_count), 'a publisher', pub, 1)
                pub_count += 1
                for topic_name, dw_ptr in std_map_values(pub['datawriter_map_']):
                    dw = deref_holder(dw_ptr)
                    dw_gp = GuidPrinter(dw['publication_id_'])
                    all_dw[dw_gp.to_plain_string()] = dw
                    set_a_var('dw_' + dw_gp.to_id_string(),
                        'a {} for {}'.format(dw.type.name, topic_name), dw, 2)

            sub_count = 0
            for sub_ptr in std_set_values(part['subscribers_']):
                sub = sub_ptr['svt_'].dereference()
                set_a_var('sub_{}_{}'.format(part_var_id, sub_count), 'a subscriber', sub, 1)
                sub_count += 1
                for topic_name, dr_ptr in std_map_values(sub['datareader_map_']):
                    dr = deref_holder(dr_ptr)
                    dr_gp = GuidPrinter(dr['subscription_id_'])
                    if dr_gp.unknown():
                        # TODO: BIT Readers?
                        continue
                    all_dr[dr_gp.to_plain_string()] = dr
                    set_a_var('dr_' + dr_gp.to_id_string(),
                        'a {} for topic {}'.format(dr.type.tag, topic_name), dr, 2)

    if len(all_part) == 1:
        set_a_var('part', 'an alias to the only domain participant', list(all_part.values())[0])

    if len(all_dw) == 1:
        set_a_var('dw', 'an alias to the only data writer', list(all_dw.values())[0])

    if len(all_dr) == 1:
        set_a_var('dr', 'an alias to the only data reader', list(all_dr.values())[0])

if part_factory:
    get_entities()
