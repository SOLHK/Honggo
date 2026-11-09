"""Execute startup/classification code from a device dylib using Unicorn.

Foundation/ObjC calls are deterministic fixtures, not Apple frameworks. This
checks the shipped ARM64 control flow and preference ownership, NOT rendering.
Usage: python tests/check_arm64_startup.py path/to/Honggo-1.1.11.dylib
Use --baseline with the exact 1.1.3 dylib to prove the two old regressions.
Requires unicorn. It does not modify or inject any host IPA.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm64_const import UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X2
from unicorn.arm64_const import UC_ARM64_REG_X3, UC_ARM64_REG_X8, UC_ARM64_REG_LR
from unicorn.arm64_const import UC_ARM64_REG_SP, UC_ARM64_REG_PC

data = Path(sys.argv[1]).read_bytes()
baseline = '--baseline' in sys.argv[2:]
u32 = lambda p: struct.unpack_from('<I', data, p)[0]
u64 = lambda p: struct.unpack_from('<Q', data, p)[0]
cstr = lambda p: data[p:data.index(b'\0', p)].decode()
assert struct.unpack_from('<IIII', data) == (0xfeedfacf, 0x100000c, 0, 6)
commands, segments, symbols, dependencies = {}, [], {}, set()
p = 32
for _ in range(u32(16)):
    command, size = struct.unpack_from('<II', data, p)
    commands[command] = p
    if command == 0x19:
        segments.append(struct.unpack_from('<4Q', data, p + 24))
    elif command == 0xc:
        dependencies.add(cstr(p + u32(p + 8)))
    elif command == 0xd:
        assert cstr(p + u32(p + 8)) == '@rpath/Honggo.dylib'
        assert u32(p + 16) == (0x10103 if baseline else 0x1010b)
    p += size
assert struct.unpack_from('<II', data, commands[0x32] + 8) == (2, 0xf0001)
assert 0x1d not in commands, 'Test dylib is unsigned and must be re-signed by the injector'
assert dependencies == {'/usr/lib/libSystem.B.dylib', '/usr/lib/libobjc.A.dylib',
    *{f'/System/Library/Frameworks/{n}.framework/{n}' for n in
      ('Foundation', 'UIKit', 'QuartzCore', 'CoreGraphics', 'CoreFoundation')}}
so, count, st, _ = struct.unpack_from('<4I', data, commands[2] + 8)
for i in range(count):
    name, _, _, _, value = struct.unpack_from('<IBBHQ', data, so + i * 16)
    if value:
        symbols[cstr(st + name)] = value
base, mock, stack = 0x1000000, 0x2000000, 0x3000000
fix = u32(commands[0x80000034] + 8)
version, starts, imp, names, count, fmt, symfmt = struct.unpack_from('<7I', data, fix)
assert (version, fmt, symfmt) == (0, 1, 0)
imports = [cstr(fix + names + (u32(fix + imp + i * 4) >> 9)) for i in range(count)]
fixups = []
s = fix + starts
for index in range(u32(s)):
    relative = u32(s + 4 + index * 4)
    if not relative:
        continue
    at = s + relative
    _, page_size, fmt, vm, _, pages = struct.unpack_from('<IHHQIH', data, at)
    assert fmt in (2, 6) and vm == segments[index][0]
    for page in range(pages):
        start = struct.unpack_from('<H', data, at + 22 + page * 2)[0]
        if start == 0xffff:
            continue
        assert start < 0x8000
        address = vm + page * page_size + start
        while True:
            v = u64(segments[index][2] + address - vm)
            target = mock + 16 * (v & 0xffffff) if v >> 63 else base + (v & ((1 << 36) - 1))
            fixups.append((base + address, target))
            step = (v >> 51) & 0xfff
            if not step:
                break
            address += 4 * step

FLAG = 'com.apple.SwiftUI.IgnoreSolariumOptOut'


class Runtime:
    def __init__(self, major=27, glass=False, controls=False, existing=False,
                 compatibility=True, available=True, empty_domain=False):
        self.uc = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
        for address, span, offset, size in segments:
            self.uc.mem_map(base + address, (span + 0xfff) & ~0xfff)
            self.uc.mem_write(base + address, data[offset:offset + size])
        for address, value in fixups:
            self.uc.mem_write(address, struct.pack('<Q', value))
        self.uc.mem_map(mock, 0x10000)
        self.uc.mem_map(stack, 0x10000)
        self.objects, self.classes = {}, {}
        self.major, self.glass, self.controls = major, glass, controls
        self.compatibility, self.available = compatibility, available
        self.empty_domain = empty_domain
        self.persistent = {FLAG: existing, 'unrelated': 42}
        self.volatile = {} if empty_domain else {'unrelatedArgument': 73, FLAG: False}
        self.writes = 0
        self.domain = self.obj('NSArgumentDomain')
        slot = mock + imports.index('_NSArgumentDomain') * 16
        self.uc.mem_write(slot, struct.pack('<Q', self.domain))
        self.defaults = self.obj('defaults')
        self.process = self.obj('process')
        self.bundle = self.obj('bundle')
        self.uc.hook_add(UC_HOOK_CODE, self.hook)

    def obj(self, value):
        address = 0x4000000 + len(self.objects) * 32
        self.objects[address] = value
        return address

    def cls(self, name):
        if name not in self.classes:
            self.classes[name] = self.obj(('class', name))
        return self.classes[name]

    def cstring(self, address):
        out = bytearray()
        while (value := self.uc.mem_read(address, 1)[0]):
            out.append(value)
            address += 1
        return out.decode()

    def string(self, address):
        if address in self.objects:
            return self.objects[address]
        _, flags, chars, length = struct.unpack('<4Q', self.uc.mem_read(address, 32))
        return self.uc.mem_read(chars, length * (2 if flags & 0x10 else 1)).decode(
            'utf-16-le' if flags & 0x10 else 'utf-8')

    def ret(self, value=0):
        self.uc.reg_write(UC_ARM64_REG_X0, int(value))
        self.uc.reg_write(UC_ARM64_REG_PC, self.uc.reg_read(UC_ARM64_REG_LR))

    def call(self, name, argument=0, boolean=0):
        self.uc.reg_write(UC_ARM64_REG_X0, argument)
        self.uc.reg_write(UC_ARM64_REG_X2, boolean)
        self.uc.reg_write(UC_ARM64_REG_LR, mock + 0xf000)
        self.uc.reg_write(UC_ARM64_REG_SP, stack + 0xf000)
        self.uc.emu_start(base + symbols[name], mock + 0xf000, count=10000)
        assert self.uc.reg_read(UC_ARM64_REG_PC) == mock + 0xf000, 'instruction limit'
        return self.uc.reg_read(UC_ARM64_REG_X0)

    def hook(self, uc, address, size, _):
        a = uc.reg_read(UC_ARM64_REG_X0)
        b = uc.reg_read(UC_ARM64_REG_X1)
        c = uc.reg_read(UC_ARM64_REG_X2)
        d = uc.reg_read(UC_ARM64_REG_X3)
        if address == base + symbols['_HGIsEnabled']:
            assert a in (0, 15)
            return self.ret(self.glass if a == 0 else self.controls)
        if address == base + symbols['_HGStyleControl']:
            # This boundary models styling, not UIKit: verify the menu invokes
            # it AFTER its own fill writes, without requiring another layout.
            value = self.objects[a]
            value['styleCalls'] += 1
            if self.controls:
                value['fill'] = None
            return self.ret()
        if not mock <= address < mock + len(imports) * 16:
            return
        name = imports[(address - mock) // 16]
        if name in ('_objc_retain', '_objc_retainAutoreleasedReturnValue',
                    '_objc_autoreleaseReturnValue', '_objc_retainAutorelease'):
            return self.ret(a)
        if name == '_objc_release':
            return self.ret()
        if name == '_sel_registerName':
            return self.ret(a)
        if name == '_NSClassFromString':
            name = self.string(a)
            return self.ret(0 if name == 'UIGlassEffect' and not self.available else self.cls(name))
        if name == '_NSStringFromClass':
            return self.ret(self.obj(self.objects[a][1]))
        if name == '_objc_msgSendSuper2':
            return self.ret()
        assert name == '_objc_msgSend', name
        if not a:
            return self.ret()
        selector = self.cstring(b)
        if selector == 'processInfo':
            return self.ret(self.process)
        if selector == 'operatingSystemVersion':
            uc.mem_write(uc.reg_read(UC_ARM64_REG_X8), struct.pack('<3q', self.major, 3, 0))
            return self.ret()
        if selector == 'standardUserDefaults':
            return self.ret(self.defaults)
        if selector == 'volatileDomainForName:':
            assert c == self.domain
            return self.ret(0 if self.empty_domain else self.obj(self.volatile))
        if selector == 'mutableCopy':
            return self.ret(self.obj(self.objects[a].copy()))
        if selector == 'dictionary':
            return self.ret(self.obj({}))
        if selector == 'numberWithBool:':
            return self.ret(self.obj(bool(c)))
        if selector == 'setObject:forKeyedSubscript:':
            self.objects[a][self.string(d)] = self.objects[c]
            return self.ret()
        if selector == 'setVolatileDomain:forName:':
            assert a == self.defaults and d == self.domain
            self.volatile = self.objects[c].copy()
            self.writes += 1
            return self.ret()
        if selector == 'boolForKey:':
            return self.ret(self.volatile.get(self.string(c), self.persistent.get(self.string(c), False)))
        if selector == 'mainBundle':
            return self.ret(self.bundle)
        if selector == 'objectForInfoDictionaryKey:':
            assert self.string(c) == 'UIDesignRequiresCompatibility'
            return self.ret(self.obj(self.compatibility) if self.compatibility is not None else 0)
        if selector == 'class':
            if a == mock + imports.index('_OBJC_CLASS_$_NSNumber') * 16:
                return self.ret(self.cls('NSNumber'))
            return self.ret(self.cls(self.objects[a][1]))
        if selector == 'hasPrefix:':
            return self.ret(self.string(a).startswith(self.string(c)))
        if selector in ('secondarySystemGroupedBackgroundColor', 'tertiarySystemFillColor'):
            return self.ret(self.obj(selector))
        if selector == 'setBackgroundColor:':
            self.objects[a]['fill'] = self.objects[c]
            return self.ret()
        if selector == 'isKindOfClass:':
            value, kind = self.objects[a], self.objects[c][1]
            return self.ret(isinstance(value, bool) if kind == 'NSNumber' else value == ('instance', kind))
        if selector == 'boolValue':
            return self.ret(self.objects[a])
        if selector == 'respondsToSelector:':
            assert self.cstring(c) == 'effectWithStyle:'
            return self.ret(self.available)
        raise AssertionError(selector)


checks = []
for major, glass, controls in [(27, True, False), (27, False, True), (26, True, True),
                                (27, False, False), (18, True, True)]:
    for existing in (False, True, None):
        for empty in (False, True):
            r = Runtime(major, glass, controls, existing, empty_domain=empty)
            before = r.persistent.copy()
            expected = major >= 26 and (glass or controls)
            r.call('_HGPrepareNativeGlass')
            assert r.call('_HGNativeGlassStartupOverride') == expected
            assert r.call('_HGNativeGlassConfigured') == expected
            assert r.persistent == before
            assert r.writes == int(expected)
            if not empty:
                assert r.volatile['unrelatedArgument'] == 73
            r.call('_HGPrepareNativeGlass')
            assert r.writes == int(expected), 'repeated startup must not rewrite the domain'
            checks.append(f'startup-{major}-{glass}-{controls}-{existing}-{empty}')
for compatibility, available in [(False, True), (True, True), (None, True), (False, False)]:
    r = Runtime(compatibility=compatibility, available=available)
    assert r.call('_HGNativeGlassConfigured') == (compatibility is False and available)
    checks.append(f'host-configuration-{compatibility}-{available}')
for kind in ('SSNestedContainerScrollView', 'SSMyUser637NestedContainerScrollView', 'UIScrollView'):
    r = Runtime()
    value = r.obj(('instance', kind))
    classifier = '_HGNestedScroll' if '_HGNestedScroll' in symbols else '_HGContainerScroll'
    assert r.call(classifier, value) == (kind != 'UIScrollView')
    checks.append(f'coordinator-{kind}')
bridge = 'RKMPandroidx.compose.ui.scene.UIKitTransparentContainerView'
for kind in (bridge + '10', bridge + '27', 'UIScrollView', 'UICollectionView',
             'SSMyUser637NestedContainerScrollView'):
    r = Runtime()
    value = r.obj(('instance', kind))
    decision = r.call('_HGNestedScroll' if baseline else '_HGContainerScroll', value)
    expected = kind == 'SSMyUser637NestedContainerScrollView' or (not baseline and kind.startswith(bridge))
    assert decision == expected
    checks.append(('old-bridge-classification-reproduced-' if baseline else 'container-') + kind)
method = next(k for k in symbols if k.endswith('[HGSettingsMenuControl setHighlighted:]'))
for enabled in (False, True):
    r = Runtime(controls=enabled)
    value = {'fill': None, 'styleCalls': 0}
    menu = r.obj(value)
    for pressed in (True, False, True, False):
        r.call(method, menu, pressed)
        expected = None if enabled and not baseline else (
            'tertiarySystemFillColor' if pressed else 'secondarySystemGroupedBackgroundColor')
        assert value['fill'] == expected
    assert value['styleCalls'] == (0 if baseline else 4)
    checks.append(('old-menu-fill-write-reproduced-' if baseline else 'menu-immediate-styling-') + str(enabled))
assert ('_HGObservedOffset' in symbols) == baseline
checks.append('scroll-offset-hook-absent' if not baseline else 'old-scroll-offset-hook-confirmed')
report = {'passed': len(checks), 'failed': 0, 'checks': checks,
          'baselineRegressionControl': baseline,
          'sha256': hashlib.sha256(data).hexdigest(),
          'scope': 'Actual dylib ARM64 control flow with mocked ObjC/Foundation boundaries; no UIKit rendering'}
print(json.dumps(report, indent=2))
