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
#   - Shortcuts for transports
#   - Printers for:
#     - IP address types (ACE_INET_Addr, NetworkAddress, any others)
#     - Time types (*TimePoint, TimeDuration, Time_t, MonotonicTime_t, and
#       ACE_Time_Value)
#   - GDB commands for getting values from standard containers using the
#     std_*_values functions

from itertools import islice
from textwrap import wrap

import gdb
import libstdcxx


gdb.execute('set print pretty on')
gdb.execute('set print static-members off')
gdb.execute('set print elements 50')


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


def value_is(value, type_name):
    resolved = value.type.strip_typedefs()
    return type_name in (value.type.name, resolved.name, value.type.tag, resolved.tag)


def _std_values(StdPrinter, value):
    for e in StdPrinter('', value).children():
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


def deref(value):
    # I'd think this should be a dynamic_cast instead of cast, but dynamic_cast
    # doesn't seem to work.
    return value.cast(value.dynamic_type).dereference()


def deref_holder(value):
    return deref(value['ptr_'])


class GuidPrinterBase:
    unsigned = gdb.lookup_type('unsigned')

    def __init__(self, array):
        self.array = array

    def unknown(self):
        return not any(self.array)

    @classmethod
    def _get_int(cls, value):
        # actual C++ type is unsigned char, which seems like it's impossible to
        # get a usable value from directly unless we cast it. base=0 is used
        # because value might be shown as decimal or hexadecimal.
        return int(str(value.cast(cls.unsigned)), base=0)

    @classmethod
    def _get_array(cls, value):
        l = value.type.strip_typedefs().range()[1] + 1
        return tuple([cls._get_int(value[i]) for i in range(0, l)])

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
        GuidPrinterBase.__init__(self, self._get_array(value))

    def to_string(self):
        return self._to_string('unknown prefix' if self.unknown() else None)


class GuidEntityPrinter(GuidPrinterBase):

    def __init__(self, value=None, entity_key=None, entity_kind=None):
        if value is not None:
            self.entity_key = self._get_array(value['entityKey'])
            self.entity_kind = self._get_int(value['entityKind'])
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
        0xc2: 'builtin keyed writer',
        0xc3: 'builtin unkeyed writer',
        0xc4: 'builtin unkeyed reader',
        0xc7: 'builtin keyed reader',
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


class GuidPrinter(GuidPrinterBase):

    def __init__(self, value):
        if value_is(value, 'OpenDDS::DCPS::GUID_t'):
            self.entity = GuidEntityPrinter(value['entityId'])
            array = GuidPrefixPrinter(value['guidPrefix']).array + self.entity.array
        elif value_is(value, 'DDS::BuiltinTopicKey_t'):
            array = self._get_array(value['value'])
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


def printer_for_type(printer, type_name):
    gdb.printing.register_pretty_printer(gdb.current_objfile(),
        lambda v: printer(v) if value_is(v, type_name) else None)


printer_for_type(GuidPrefixPrinter, 'OpenDDS::DCPS::GuidPrefix_t')
printer_for_type(GuidEntityPrinter, 'OpenDDS::DCPS::EntityId_t')
printer_for_type(GuidPrinter, 'OpenDDS::DCPS::GUID_t')
printer_for_type(GuidPrinter, 'DDS::BuiltinTopicKey_t')


def set_a_var(name, desc, value, indent=0):
    i = lambda l: '  ' * l
    print(*wrap('- ${} is {}'.format(name, desc), width=line_len,
        initial_indent=i(indent), subsequent_indent=i(indent + 1)), sep='\n')
    # Set it both in GDB and Python
    gdb.set_convenience_variable(name, value)
    globals()[name] = value


set_a_var('service_part', 'the service participant', gdb.parse_and_eval(
    'ACE_Singleton<OpenDDS::DCPS::Service_Participant,ACE_Thread_Mutex>::singleton_->instance_'))
set_a_var('part_factory', 'the participant factory',
    deref_holder(service_part['dp_factory_servant_']))
set_a_var('transport_reg', 'the transport registry', gdb.parse_and_eval(
    'ACE_Unmanaged_Singleton<OpenDDS::DCPS::TransportRegistry,ACE_Recursive_Thread_Mutex>::singleton_->instance_'))


header('Discoveries', '-')

for name, disc_rc in std_map_values(service_part['discoveryMap_']):
    disc = deref_holder(disc_rc)
    set_a_var('disc_' + str(name).strip('"'), 'a ' + disc.type.name, disc)

    if disc.type.name == 'OpenDDS::RTPS::RtpsDiscovery':
        for domain, part_map in std_map_values(disc['participants_']):
            for part_guid, part_handle in std_map_values(part_map):
                spdp = deref_holder(part_handle)
                set_a_var('spdp_' + GuidPrinter(part_guid).to_id_string(),
                    'a ' + spdp.type.tag, spdp, 1)
                sedp = deref_holder(spdp['sedp_'])
                set_a_var('sedp_' + GuidPrinter(part_guid).to_id_string(),
                    'a ' + sedp.type.tag, sedp, 1)


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
